/*
 * Copyright (c) 2013-2018 Balabit
 * Copyright (c) 2013 Gergely Nagy <algernon@balabit.hu>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * As an additional exemption you are allowed to compile & link against the
 * OpenSSL libraries as published by the OpenSSL project. See the file
 * COPYING for details.
 *
 */
#include <stdlib.h>

#include <criterion/criterion.h>
#include "parse-number.h"

static void
assert_parse_with_suffix(const gchar *str, gint64 expected)
{
  gint64 n;
  gboolean res;

  res = parse_number_with_suffix(str, &n);

  cr_assert_eq(res,TRUE,"Parsing (w/ suffix) %s failed", str);
  cr_assert_eq(n,expected,"Parsing (w/ suffix) %s failed", str);
}

static void
assert_parse_with_suffix_fails(const gchar *str)
{
  gint64 n;
  gboolean res;

  res = parse_number_with_suffix(str, &n);
  cr_assert_eq(res,FALSE, "Parsing (w/ suffix) %s succeeded, while expecting failure", str);
}

static void
assert_parse(const gchar *str, gint64 expected)
{
  gint64 n;
  gboolean res;

  res = parse_number(str, &n);

  cr_assert_eq(res, TRUE, "Parsing (w/o suffix) %s failed", str);
  cr_assert_eq(n, expected, "Parsing (w/o suffix) %s failed", str);
}

static void
assert_parse_fails(const gchar *str)
{
  gint64 n;
  gboolean res;

  res = parse_number(str, &n);

  cr_assert_eq(res, FALSE, "Parsing (w/o suffix) %s succeeded, while expecting failure", str);
}

Test(parse_number, test_simple_numbers_are_parsed_properly)
{
  assert_parse_with_suffix("1234", 1234);
  assert_parse_with_suffix("+1234", 1234);
  assert_parse_with_suffix("-1234", -1234);

  assert_parse("1234", 1234);
}

Test(parse_number,test_c_like_prefixes_select_base)
{
  assert_parse("0x20", 32);
  assert_parse("020", 16);
  assert_parse("20", 20);
  assert_parse_with_suffix("0x20kiB", 32 * 1024);
}

Test(parse_number,test_exponent_suffix_is_parsed_properly)
{
  assert_parse_with_suffix("1K", 1000);
  assert_parse_with_suffix("1k", 1000);
  assert_parse_with_suffix("1m", 1000 * 1000);
  assert_parse_with_suffix("1M", 1000 * 1000);
  assert_parse_with_suffix("1G", 1000 * 1000 * 1000);
  assert_parse_with_suffix("1g", 1000 * 1000 * 1000);
}

Test(parse_number,test_byte_units_are_accepted)
{
  assert_parse_with_suffix("1b", 1);
  assert_parse_with_suffix("1B", 1);
  assert_parse_with_suffix("1Kb", 1000);
  assert_parse_with_suffix("1kB", 1000);
  assert_parse_with_suffix("1mb", 1000 * 1000);
  assert_parse_with_suffix("1MB", 1000 * 1000);
  assert_parse_with_suffix("1Gb", 1000 * 1000 * 1000);
  assert_parse_with_suffix("1gB", 1000 * 1000 * 1000);
}

Test(parse_number,test_base2_is_selected_by_an_i_modifier)
{
  assert_parse_with_suffix("1Kib", 1024);
  assert_parse_with_suffix("1kiB", 1024);
  assert_parse_with_suffix("1Ki", 1024);
  assert_parse_with_suffix("1kI", 1024);
  assert_parse_with_suffix("1mib", 1024 * 1024);
  assert_parse_with_suffix("1MiB", 1024 * 1024);
  assert_parse_with_suffix("1Gib", 1024 * 1024 * 1024);
  assert_parse_with_suffix("1giB", 1024 * 1024 * 1024);
  assert_parse_with_suffix("1024giB", 1024LL * 1024LL * 1024LL * 1024LL);
}

Test(parse_number,test_invalid_formats_are_not_accepted)
{
  assert_parse_with_suffix_fails("1234Z");
  assert_parse_with_suffix_fails("1234kZ");
  assert_parse_with_suffix_fails("1234kdZ");
  assert_parse_with_suffix_fails("1234kiZ");
  assert_parse_fails("1234kiZ");
}
