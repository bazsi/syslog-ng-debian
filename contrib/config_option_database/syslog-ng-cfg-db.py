#!/usr/bin/env python3
#############################################################################
# Copyright (c) 2019 Balabit
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 2 as published
# by the Free Software Foundation, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#
# As an additional exemption you are allowed to compile & link against the
# OpenSSL libraries as published by the OpenSSL project. See the file
# COPYING for details.
#
#############################################################################

from pathlib import Path
import json

from utils.ConfigOptions import get_driver_options
from argparse import ArgumentParser


def parse_args():
    parser = ArgumentParser()
    parser.add_argument('--context', '-c', type=str, help='e.g.: destination')
    parser.add_argument('--driver', '-d', type=str, help='e.g.: http')
    parser.add_argument('--rebuild', '-r', action='store_true',
                        help='Use this flag, if there is a modification in either '
                             'the DB generating script, or the grammar files')
    args = parser.parse_args()
    return args


def merge_block_stored_as_an_option(keyword, arguments, options, blocks):
    options.remove([keyword, arguments])
    if ('', arguments) not in blocks[keyword]['options']:
        blocks[keyword]['options'].append(['', arguments])


def merge_blocks_stored_as_options_helper(db_node):
    options = db_node['options']
    blocks = db_node['blocks']

    for keyword, arguments in options.copy():
        if keyword in blocks:
            merge_block_stored_as_an_option(keyword, arguments, options, blocks)

    for block in blocks.values():
        merge_blocks_stored_as_options_helper(block)


def merge_blocks_stored_as_options(db):
    for context in db:
        for driver in db[context]:
            merge_blocks_stored_as_options_helper(db[context][driver])


def build_db():
    db = {}
    for context, driver, keyword, arguments, parents in get_driver_options():
        arguments = list(arguments)
        db.setdefault(context, {})
        driver_options = {'options': [], 'blocks': {}}
        for driver_alias in driver.split('/'):
            db[context].setdefault(driver_alias, driver_options)
        db_node = db[context][driver_alias]
        for parent in parents:
            db_node['blocks'].setdefault(parent, {'options': [], 'blocks': {}})
            db_node = db_node['blocks'][parent]
        db_node['options'].append([keyword, arguments])
    merge_blocks_stored_as_options(db)
    return db


def get_db(rebuild):
    cache_file = Path(__file__).parents[0] / '.cache' / 'options.json'
    Path.mkdir(cache_file.parents[0], exist_ok=True)
    if rebuild or not cache_file.exists():
        db = build_db()
        with cache_file.open('w') as f:
            json.dump(db, f, indent=2)
    else:
        with cache_file.open() as f:
            db = json.load(f)
    return db


def is_option_with_multiple_signature(option):
    _, arguments_list = option
    return len(arguments_list) > 1


def merge_mergeable_options(mergeable_options, options):
    for keyword, arguments_list in mergeable_options:
        merged_arguments = []
        for arguments in sorted(arguments_list):
            if merged_arguments:
                merged_arguments.append('|')
            merged_arguments.extend(arguments)

        for arguments in arguments_list:
            options.remove([keyword, arguments])

        options.append([keyword, merged_arguments])


def normalize_options(options):
    normalized_options = options.copy()
    options_groupped = {}

    for keyword, arguments in options:
        if keyword:
            options_groupped.setdefault(keyword, []).append(arguments)
    mergeable_options = filter(is_option_with_multiple_signature, options_groupped.items())
    merge_mergeable_options(mergeable_options, normalized_options)
    return sorted(normalized_options)


def normalize_arguments(arguments):
    if not arguments:
        arguments = ['']
    arguments = map(lambda arg: arg if arg else '<empty>', arguments)
    return arguments


def print_options_helper(block, depth):
    options = block['options']
    blocks = block['blocks']
    indent = '  ' * depth
    for keyword, arguments in normalize_options(options):
        arguments = normalize_arguments(arguments)
        if keyword:
            print(indent + '{}({})'.format(keyword, ' '.join(arguments)))
        else:
            print(indent + '{}'.format(' '.join(arguments)))
    for key in sorted(blocks.keys()):
        print(indent + '{}('.format(key))
        print_options_helper(blocks[key], depth + 1)
        print(indent + ')')


def print_options(db, context, driver):
    if context not in db:
        print('The context "{}" is not in the database.'.format(context))
        print_contexts(db)
    elif driver not in db[context]:
        print('The driver "{}" is not in the drivers of context "{}".'.format(driver, context))
        print_drivers(db, context)
    else:
        print('{} {}('.format(context, driver))
        print_options_helper(db[context][driver], 1)
        print(')')


def print_drivers(db, context):
    if context in db:
        print('Drivers of context "{}":'.format(context))
        for driver in sorted(db[context]):
            print('  {}'.format(driver))
        print('Print the options of DRIVER with `--context {} --driver DRIVER`.'.format(context))
    else:
        print('The context "{}" is not in the database.'.format(context))
        print_contexts(db)


def print_contexts(db):
    print('Valid contexts:')
    for context in sorted(db):
        print('  {}'.format(context))
    print('Print the drivers of CONTEXT with `--context CONTEXT`.')


def main():
    args = parse_args()
    db = get_db(args.rebuild)
    if args.context and args.driver:
        print_options(db, args.context, args.driver)
    elif args.context:
        print_drivers(db, args.context)
    elif args.driver:
        print('Please define the context of "{}" with `--context CONTEXT`.'.format(args.driver))
    elif not args.rebuild:
        print_contexts(db)


if __name__ == '__main__':
    main()
