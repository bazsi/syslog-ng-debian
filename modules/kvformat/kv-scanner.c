#include "kv-scanner.h"
#include "misc.h"
#include <string.h>

enum {
  KV_QUOTE_INITIAL = 0,
  KV_QUOTE_STRING,
  KV_QUOTE_BACKSLASH,
  KV_QUOTE_FINISH
};

void
kv_scanner_input(KVScannerState *self, const gchar *input)
{
  self->input = input;
  self->input_len = strlen(input);
  self->input_pos = 0;
}

static gboolean
_kv_scanner_skip_space(KVScannerState *self)
{
  while (self->input[self->input_pos] == ' ')
    self->input_pos++;
  return TRUE;
}

static gboolean
_is_valid_key_character(int c)
{
  return (c >= 'a' && c <= 'z') ||
         (c >= 'A' && c <= 'Z') ||
         (c >= '0' && c <= '9') ||
         (c == '_') ||
         (c == '-');
}

static gboolean
_kv_scanner_extract_key(KVScannerState *self)
{
  const gchar *input_ptr = &self->input[self->input_pos];
  const gchar *start_of_key;
  const gchar *equal;

  equal = strchr(input_ptr, '=');
  if (!equal)
    return FALSE;
  start_of_key = equal - 1;
  while (start_of_key > input_ptr && _is_valid_key_character(*start_of_key))
    start_of_key--;
  if (!_is_valid_key_character(*start_of_key))
    start_of_key++;
  g_string_assign_len(self->key, start_of_key, equal - start_of_key);
  self->input_pos = equal - self->input + 1;
  return TRUE;
}

static gboolean
_kv_scanner_extract_value(KVScannerState *self)
{
  const gchar *cur;
  gchar control;

  g_string_truncate(self->value, 0);
  self->quote_state = KV_QUOTE_INITIAL;
  cur = &self->input[self->input_pos];
  while (*cur && self->quote_state != KV_QUOTE_FINISH)
    {
      switch (self->quote_state)
        {
        case KV_QUOTE_INITIAL:
          if (*cur == ' ' || strncmp(cur, ", ", 2) == 0)
            {
              self->quote_state = KV_QUOTE_FINISH;
              break;
            }
          else if (*cur == '\"' || *cur == '\'')
            {
              self->quote_state = KV_QUOTE_STRING;
              self->quote_char = *cur;
              break;
            }
          g_string_append_c(self->value, *cur);
          break;
        case KV_QUOTE_STRING:
          if (*cur == self->quote_char)
            {
              self->quote_state = KV_QUOTE_INITIAL;
              break;
            }
          else if (*cur == '\\')
            {
              self->quote_state = KV_QUOTE_BACKSLASH;
              break;
            }
          g_string_append_c(self->value, *cur);
          break;
        case KV_QUOTE_BACKSLASH:
          switch (*cur)
            {
            case 'b':
              control = '\b';
              break;
            case 'f':
              control = '\f';
              break;
            case 'n':
              control = '\n';
              break;
            case 'r':
              control = '\r';
              break;
            case 't':
              control = '\t';
              break;
            case '\\':
              control = '\\';
              break;
            default:
              if (self->quote_char != *cur)
                g_string_append_c(self->value, '\\');
              control = *cur;
              break;
            }
          g_string_append_c(self->value, control);
          self->quote_state = KV_QUOTE_STRING;
          break;
        }
      cur++;
    }
  self->input_pos = cur - self->input;
  return TRUE;
}

gboolean
kv_scanner_scan_next(KVScannerState *self)
{
  _kv_scanner_skip_space(self);
  if (!_kv_scanner_extract_key(self) ||
      !_kv_scanner_extract_value(self))
    return FALSE;

  return TRUE;
}

const gchar *
kv_scanner_get_current_key(KVScannerState *self)
{
  return self->key->str;
}

const gchar *
kv_scanner_get_current_value(KVScannerState *self)
{
  return self->value->str;
}

void
kv_scanner_init(KVScannerState *self)
{
  memset(self, 0, sizeof(*self));
  self->key = g_string_sized_new(32);
  self->value = g_string_sized_new(64);
}

void
kv_scanner_destroy(KVScannerState *self)
{
  g_string_free(self->key, TRUE);
  g_string_free(self->value, TRUE);
}
