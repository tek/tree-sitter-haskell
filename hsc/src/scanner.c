#define HSC_EXT

#include "../../src/scanner.c"

// Override vanilla symop_lookahead for hsc purposes.
static uint32_t symop_lookahead(Env *env) {
  if (env->symop == 0) {

    uint32_t i = 0;

    env->symop = advance_while(env, 0, symop_char);

    // Loop either over either symop characters or pairs of hashes.
    while (symop_char(peek(env, i))) {
      // In hsc only duplicated # are part of operators. Regular # are
      // reserved for hsc directives.
      if (char_at(env, i, '#')) {
        uint32_t c = peek(env, i + 1);
        if (c == '#') {
          i += 2;
        }
        else if (c == '{') {
          break;
        }
        else {
          i++;
        }
      } else {
        i++;
      }
    }

    env->symop = i;
    if (env->symop > 0)
      dbg("symop: %d, %.*ls\n", env->symop, env->symop, array_get(&env->state->lookahead, env->state->offset));
  }
  return env->symop;
}

static Lexed lex_hsc_hash(Env *env) {
  uint32_t c2 = peek1(env);
  switch (c2) {
  case '#': {
    int32_t c3 = peek2(env);
    switch (c3) {
    case ')':
      return LUnboxedClose;
    case '|':
      // Unboxed sum with missing space `(#| Int #)` in hsc mode so it actually looks like `(##| Int #)`
      return LSymopSpecial;
    case '#':
      if (char_at(env, 3, '#')) {
        // Unboxed unit `(##)` and hsc mode so it actually looks like `(####)`
        return LSymopSpecial;
      }
      break;
    default:
      return LHash;
    }
    return (c3 == ')') ? LUnboxedClose : LHash;
    break;
  }
  case '|':
    return LSymopSpecial;
  default:
    return LHscHash;
  }
}

// hsc2hs directive with arbitrary C expressions

static Symbol hsc_args(Env *env, Symbol sym, bool directive_ends_with_newline) {
  int depth = directive_ends_with_newline ? 0 : 1;
  bool is_escaped = false;
  bool in_string = false;
  bool in_char = false;
  bool in_line_comment = false;
  bool in_region_comment = false;
  for (;;) {
    if (is_eof(env)) {
      return finish_marked(env, sym, "hsc_args");
    }

    int32_t c = PEEK;
    switch (c) {
      case '\'':
        if (is_escaped) {
          is_escaped = false;
        }
        else if (in_string || in_line_comment || in_region_comment) {
          // Skip the character within string literal.
        }
        else {
          in_char = !in_char;
        }
        break;
      case '"':
        if (is_escaped) {
          is_escaped = false;
        }
        else if (in_char || in_line_comment || in_region_comment) {
          // Skip the contents of character literal.
        }
        else {
          in_string = !in_string;
        }
        break;
      case '(':
      case '[':
      case '{':
        if (!in_string && !in_char && !is_escaped && !in_line_comment && !in_region_comment) {
          depth++;
        }
        break;
      case ')':
      case ']':
      case '}':
        if (!in_string && !in_char && !is_escaped && !in_line_comment && !in_region_comment) {
          depth--;
          if (directive_ends_with_newline ? depth < 0 : depth <= 0) {
            return finish_marked(env, sym, "hsc_args");
          }
        }
        break;
      case '\\':
        if (!in_line_comment && !in_region_comment) {
          is_escaped = true;
        }
        break;
      case '\r':
        break;
      case '\n':
        if (in_line_comment) {
          in_line_comment = false;
        }
        else if (in_region_comment) {
          // Consume newline while we’re in comment.
        }
        else if (is_escaped) {
          is_escaped = false;
        }
        else if (directive_ends_with_newline && depth <= 0) {
          return finish_marked(env, sym, "hsc_args");
        }
        break;
      case '/':
        if (!in_line_comment && !in_region_comment) {
          S_ADVANCE;
          int32_t next = PEEK;
          switch (next) {
          case '/':
            in_line_comment = true;
            break;
          case '*':
            in_region_comment = true;
            break;
          default:
            // Nothing to do
          }
        }
        break;
      case '*': {
        if (in_region_comment) {
          S_ADVANCE;
          if (PEEK == '/') {
            in_region_comment = false;
          }
        } else {
          is_escaped = false;
        }
        break;
      }
      default:
        is_escaped = false;
    }

    S_ADVANCE;
  }
}

void *tree_sitter_hsc_external_scanner_create() {
  return tree_sitter_haskell_external_scanner_create();
}

bool tree_sitter_hsc_external_scanner_scan(void *payload, TSLexer *lexer, const bool *valid_symbols) {
  return tree_sitter_haskell_external_scanner_scan(payload, lexer, valid_symbols);
}

unsigned tree_sitter_hsc_external_scanner_serialize(void *payload, char *buffer) {
  return tree_sitter_haskell_external_scanner_serialize(payload, buffer);
}

void tree_sitter_hsc_external_scanner_deserialize(void *payload, const char *buffer, unsigned length) {
  return tree_sitter_haskell_external_scanner_deserialize(payload, buffer, length);
}

void tree_sitter_hsc_external_scanner_destroy(void *payload) {
  return tree_sitter_haskell_external_scanner_destroy(payload);
}
