#!/usr/bin/env python
#
# Copyright (c) 2014
#	Yoshitaro Makise
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

from __future__ import print_function
import io
import os
import subprocess
import sys
import re
import string
import optparse
import pygments.lexers
from pygments.token import Token

EXUBERANT_CTAGS = "../../../bin/ctags.exe"

# In most cases, lexers can be looked up with lowercase form of formal
# language names. This dictionary defines exceptions.
LANGUAGE_ALIASES = {
    'fantom':     'fan',
    'haxe':       'haXe',
    'sourcepawn': 'sp',
    'typescript': 'ts',
    'xbase':      'XBase'
}

# All punctuation characters except some characters which are valid
# identifier characters in some languages
if sys.version_info < (3,):
    PUNCTUATION_CHARACTERS = string.punctuation.translate(None, '-_.')
else:
    PUNCTUATION_CHARACTERS = string.punctuation.translate(str.maketrans('', '', '-_.'))

CLOSEFDS = sys.platform != 'win32';

TERMINATOR = '###terminator###\n'

class ParserOptions:
    def __init__(self):
        self.strip_punctuation = False

class PygmentsParser:
    class ContentParser:
        def __init__(self, path, text, lexer, options):
            self.path = path
            self.text = text
            self.lexer = lexer
            self.options = options
            self.lines_index = None

        def parse(self):
            self.lines_index = self.build_lines_index(self.text)
            tokens = self.lexer.get_tokens_unprocessed(self.text)
            return self.parse_tokens(tokens)

        # builds index of beginning of line
        def build_lines_index(self, text):
            lines_index = []
            cur = 0
            while True:
                i = text.find('\n', cur)
                if i == -1:
                    break
                cur = i + 1
                lines_index.append(cur)
            lines_index.append(len(text))    # sentinel
            return lines_index

        def parse_tokens(self, tokens):
            result = {}
            cur_line = 0
            for index, tokentype, tag in tokens:
                if tokentype in Token.Name:
                    # we can assume index are delivered in ascending order
                    while self.lines_index[cur_line] <= index:
                        cur_line += 1
                    tag = re.sub(r'\s+', '', tag)    # remove newline and spaces
                    if self.options.strip_punctuation:
                        tag = tag.strip(PUNCTUATION_CHARACTERS)
                    if tag:
                        result[(False, tag, cur_line + 1)] = ''
            return result

    def __init__(self, langmap, options):
        self.langmap = langmap
        self.options = options

    def parse(self, path):
        lexer = self.get_lexer_by_langmap(path)
        if lexer:
            text = self.read_file(path)
            if text:
                parser = self.ContentParser(path, text, lexer, self.options)
                return parser.parse()
        return {}

    def get_lexer_by_langmap(self, path):
        ext = os.path.splitext(path)[1]
        if sys.platform == 'win32':
            lang = self.langmap.get(ext.lower(), None)
        else:
            lang = self.langmap.get(ext, None)
        if lang:
            name = lang.lower()
            if name in LANGUAGE_ALIASES:
                name = LANGUAGE_ALIASES[name]
            lexer = pygments.lexers.get_lexer_by_name(name)
            return lexer
        return None

    def read_file(self, path):
        try:
            if sys.version_info < (3,):
                with open(path, 'r') as f:
                    text = f.read()
                    return text
            else:
                with open(path, 'r', encoding='latin1') as f:
                    text = f.read()
                    return text
        except Exception as e:
            print(e, file=sys.stderr)
            return None

class CtagsParser:
    def __init__(self, ctags_command, options):
        self.process = subprocess.Popen([ctags_command, '-xu', '--filter', '--filter-terminator=' + TERMINATOR, '--format=1'], bufsize=-1,
                                        stdin=subprocess.PIPE, stdout=subprocess.PIPE, close_fds=CLOSEFDS,
                                        universal_newlines=True)
        if sys.version_info < (3,):
            self.child_stdout = self.process.stdout
        else:
            self.child_stdout = io.TextIOWrapper(self.process.stdout.buffer, encoding='latin1')
            sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='latin1')
        self.child_stdin = self.process.stdin
        self.options = options

    def parse(self, path):
        print(path, file=self.child_stdin)
        self.child_stdin.flush()
        result = {}
        while True:
            line = self.child_stdout.readline()
            if not line or line.startswith(TERMINATOR):
                break
            match = re.search(r'(\S+)\s+(\d+)\s+' + re.escape(path) + r'\s+(.*)$', line)
            if match:
                (tag, lnum, image) = match.groups()
                if self.options.strip_punctuation:
                    tag = tag.strip(PUNCTUATION_CHARACTERS)
                if tag:
                    result[(True, tag, int(lnum))] = image
        return result

class MergingParser:
    def __init__(self, def_parser, ref_parser):
        self.def_parser = def_parser
        self.ref_parser = ref_parser
        pass

    def parse(self, path):
        def_result = self.def_parser.parse(path)
        ref_result = self.ref_parser.parse(path)
        result = def_result.copy()
        result.update(ref_result)
        for (isdef, tag, lnum) in def_result:
            ref_entry = (False, tag, lnum)
            if ref_entry in ref_result:
                del result[ref_entry]
        return result

def parse_langmap(string):
    langmap = {}
    mappings = string.split(',')
    for mapping in mappings:
        lang, exts = mapping.split(':')
        if not lang[0].islower():  # skip lowercase, that is for builtin parser
            for ext in exts.split('.'):
                if ext:
                    if sys.platform == 'win32':
                        langmap['.' + ext.lower()] = lang
                    else:
                        langmap['.' + ext] = lang
    return langmap

def handle_requests(langmap, options):
    global EXUBERANT_CTAGS
    if EXUBERANT_CTAGS != '' and EXUBERANT_CTAGS != 'no':
        ctags_bin = os.path.join(os.path.dirname(__file__), EXUBERANT_CTAGS)
        pygments_parser = PygmentsParser(langmap, options)
        try:
            ctags_parser = CtagsParser(ctags_bin, options)
            parser = MergingParser(ctags_parser, pygments_parser)
        except Exception as e:
            parser = pygments_parser
    else:
        parser = PygmentsParser(langmap, options)
    while True:
        path = sys.stdin.readline()
        if not path:
            break
        path = path.rstrip()
        tags = parser.parse(path)
        for (isdef, tag, lnum),image in tags.items():
            if isdef:
                typ = 'D'
            else:
                typ = 'R'
            print(typ, tag, lnum, path, image)
        print(TERMINATOR, end='')
        sys.stdout.flush()

def get_parser_options_from_env(parser_options):
    env = os.getenv('GTAGSPYGMENTSOPTS')
    if env:
        for s in env.split(','):
            s = s.strip()
            if s == 'strippunctuation':
                parser_options.strip_punctuation = True

def main():
    opt_parser = optparse.OptionParser()
    opt_parser.add_option('--langmap', dest='langmap')
    (options, args) = opt_parser.parse_args()
    if not options.langmap:
        opt_parser.error('--langmap option not given')
    langmap = parse_langmap(options.langmap)
    parser_options = ParserOptions()
    get_parser_options_from_env(parser_options)
    handle_requests(langmap, parser_options)

if __name__ == '__main__':
    main()
