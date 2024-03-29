#define DEBUG 0

#include "tree_sitter/parser.h"
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <wctype.h>
#include <locale.h>
#include <stdio.h>
#include "id.h"
#include "space.h"
#include "varid-start.h"
#include "symop.h"

#define PEEK env->lexer->lookahead

#if DEBUG

#define S_ADVANCE advance_debug()
#define S_SKIP skip_debug()
#define MARK(s) mark_debug(s)
#define dbg(...) do { fprintf(stderr, __VA_ARGS__); } while (0)

#else

// Move the parser position one character to the right.
#define S_ADVANCE advance()

// Move the parser position one character to the right, treating the consumed character as whitespace.
#define S_SKIP env->lexer->advance(env->lexer, true)

/**
 * Instruct the lexer that the current position is the end of the potentially detected symbol, causing the next run to
 * be started after this character in the success case.
 *
 * This is useful if the validity of the detected symbol depends on what follows.
 */
#define MARK(s) env->lexer->mark_end(env->lexer)

#define dbg(...) do {} while (0)

#endif

#define SEQ(expr) do { Symbol res = expr; if (res) return res; } while (0)
#define SEQT(expr) do { Lexed res = expr; if (res != LNothing) return res; } while (0)

// --------------------------------------------------------------------------------------------------------
// Vector
// --------------------------------------------------------------------------------------------------------

#define VEC_RESIZE(vec, new_cap) {\
  (vec)->data = realloc((vec)->data, (new_cap) * sizeof((vec)->data[0])); \
  assert((vec)->data != NULL); \
  (vec)->cap = (new_cap); }

#define VEC_GROW(vec, new_cap) if ((vec)->cap < (new_cap)) { VEC_RESIZE((vec), (new_cap)); }

#define VEC_PUSH(vec, el) {\
  if ((vec)->cap == (vec)->len) { VEC_RESIZE((vec), (vec)->len > 0 ? (vec)->len * 2 : 8); } \
  (vec)->data[(vec)->len++] = (el); }

#define VEC_POP(vec) (vec)->len--;

#define VEC_BACK(vec) ((vec)->data[(vec)->len - 1])

#define VEC_FREE(vec) { if ((vec)->data != NULL) free((vec)->data); }

// --------------------------------------------------------------------------------------------------------
// Symbols
// --------------------------------------------------------------------------------------------------------

typedef enum {
  FAIL,
  SEMICOLON,
  START,
  START_DO,
  START_CASE,
  START_IF,
  START_LET,
  START_QUOTE,
  END,
  START_BRACE,
  END_BRACE,
  START_TEXP,
  END_TEXP,
  WHERE,
  IN,
  ARROW,
  BAR,
  DERIVING,
  COMMENT,
  HADDOCK,
  CPP,
  PRAGMA,
  QQ_START,
  QQ_BODY,
  SPLICE,
  QUAL_DOT,
  TIGHT_DOT,
  PREFIX_DOT,
  DOTDOT,
  TIGHT_AT,
  PREFIX_AT,
  TIGHT_BANG,
  PREFIX_BANG,
  TIGHT_TILDE,
  PREFIX_TILDE,
  PREFIX_PERCENT,
  VARSYM,
  CONSYM,
  UPDATE,
} Symbol;

#if DEBUG

static const char *sym_names[] = {
  "fail",
  "semicolon",
  "start",
  "start_do",
  "start_case",
  "start_if",
  "start_let",
  "start_quote",
  "end",
  "start_brace",
  "end_brace",
  "start_texp",
  "end_texp",
  "where",
  "in",
  "arrow",
  "bar",
  "deriving",
  "comment",
  "haddock",
  "cpp",
  "pragma",
  "qq_start",
  "qq_body",
  "splice",
  "tight_dot",
  "proj_dot",
  "prefix_dot",
  "dotdot",
  "tight_at",
  "prefix_at",
  "tight_bang",
  "prefix_bang",
  "tight_tilde",
  "prefix_tilde",
  "prefix_percent",
  "varsym",
  "consym",
  "indent",
};

#endif

// --------------------------------------------------------------------------------------------------------
// Data
// --------------------------------------------------------------------------------------------------------

#if DEBUG
static char const *context_names[] = {

  "decls",
  "do",
  "case",
  "let",
  "multi_way_if",
  "quote",
  "braces",
  "texp",
  "module_header",
  "none",
};

typedef struct {
  unsigned len;
  unsigned cap;
  int32_t *data;
} ParseLine;

typedef struct {
  unsigned len;
  unsigned cap;
  ParseLine *data;
} ParseLines;

typedef struct {
  int marked;
  unsigned marked_line;
  unsigned start_col;
  unsigned start_line;
  unsigned end_col;
  const char *marked_by;
} Debug;

Debug debug_new(TSLexer *l) {
  return (Debug) {
    .marked = -1,
    .marked_line = 0,
    .start_col = l->get_column(l),
    .start_line = 0,
    .end_col = 0,
    .marked_by = "",
  };
}

#endif

typedef enum {
  DeclLayout,
  DoLayout,
  CaseLayout,
  LetLayout,
  QuoteLayout,
  MultiWayIfLayout,
  Braces,
  TExp,
  ModuleHeader,
  NoContext,
} ContextSort;

typedef struct {
  ContextSort sort;
  uint32_t indent;
} Context;

typedef enum {
  LNothing,
  LEof,
  LWhere,
  LIn,
  LThen,
  LElse,
  LDeriving,
  LModule,
  LTick,
  LSymop,
  LDotDot,
  LDotId,
  LDotSymop,
  LDotOpen,
  LDollar,
  LBang,
  LTilde,
  LAt,
  LPercent,
  LHash,
  LBar,
  LArrow,
  LTexpCloser,
  LQuoteClose,
  LPragma,
  LBlockComment,
  LLineComment,
  LBraceClose,
  LBraceOpen,
  LBracketOpen,
  LUnboxedClose,
  LSemi,
  LCppElse,
  LCpp,
} Lexed;

#if DEBUG

static const char *token_names[] = {
  "nothing",
  "eof",
  "where",
  "in",
  "then",
  "else",
  "deriving",
  "module",
  "tick",
  "symop",
  "dot-dot",
  "dot-id",
  "dot-symop",
  "dot-open",
  "dollar",
  "bang",
  "tilde",
  "at",
  "percent",
  "hash",
  "bar",
  "arrow",
  "texp-closer",
  "quote-close",
  "pragma",
  "block-comment",
  "line-comment",
  "brace-close",
  "brace-open",
  "bracket-open",
  "unboxed-close",
  "semi",
  "cpp-else",
  "cpp",
};

#endif

typedef enum {
  NInactive,
  NInit,
  NProcess,
  NResume,
} NewlineState;

typedef struct {
  NewlineState state;
  uint32_t indent;
  Lexed end;
  bool eof;
  bool no_semi;
  bool skip_semi;
  bool unsafe;
} Newline;

typedef struct {
  uint32_t len;
  uint32_t cap;
  Context *data;
} Contexts;

typedef struct {
  uint32_t len;
  uint32_t cap;
  int32_t *data;
  uint32_t offset;
} Lookahead;

typedef struct {
  Contexts contexts;
  bool disabled;
  Newline newline;
  Lookahead *lookahead;
#if DEBUG
  ParseLines parse;
#endif
} State;

/**
 * This structure contains the external and internal state.
 *
 * The parser provides the lexer interface and the list of valid symbols.
 *
 * The internal state consists of a stack of indentation widths that is manipulated whenever a layout is started or
 * terminated.
 */
typedef struct {
  TSLexer *lexer;
  const bool *symbols;
  uint32_t symop;
#if DEBUG
  Debug debug;
#endif
} Env;

Env env_new(TSLexer *l, const bool * symbols) {
  return (Env) {
    .lexer = l,
    .symbols = symbols,
    .symop = 0,
#if DEBUG
    .debug = debug_new(l),
#endif
  };
}

static Env *env;
static State *state;
static Contexts *contexts;
static Newline *newline;

static void reset_newline() { memset(newline, 0, sizeof(Newline)); }

static bool newline_active() { return newline->state == NInit || newline->state == NProcess; }

static bool newline_init() { return newline->state == NInit; }

// --------------------------------------------------------------------------------------------------------
// Lexer interaction
// --------------------------------------------------------------------------------------------------------

static bool is_eof() { return env->lexer->eof(env->lexer); }

static bool not_eof() { return !(is_eof()); }

/**
 * The parser's position in the current line.
 * Note: This is expensive to use.
 */
static uint32_t column() {
  return is_eof() ? 0 : env->lexer->get_column(env->lexer);
}

/**
 * Don't add zeroes to the lookahead buffer when hitting EOF – it causes `no_lookahead` to report false negatives.
 */
static void advance() {
  if (not_eof()) {
    VEC_PUSH(state->lookahead, PEEK);
    env->lexer->advance(env->lexer, false);
  }
}

static bool set_result_symbol(Symbol result) {
  if (result != FAIL) {
    env->lexer->result_symbol = (TSSymbol) result;
    return true;
  }
  else return false;
}

#if DEBUG

static void mark_debug(const char *restrict marked_by) {
  dbg("mark: %s\n", marked_by);
  env->debug.marked = (int) column();
  env->debug.marked_line = 0;
  env->debug.marked_by = marked_by;
  env->lexer->mark_end(env->lexer);
}

static void append_parse_buffer();

static void advance_debug() {
  append_parse_buffer();
  advance();
}

static void skip_debug() {
  append_parse_buffer();
  env->lexer->advance(env->lexer, true);
}

#endif

/**
 * `inline` has a noticeable impact, reaching parity with a macro.
 */
static inline bool valid(Symbol s) { return env->symbols[s]; }

// --------------------------------------------------------------------------------------------------------
// Symbol constructors
// --------------------------------------------------------------------------------------------------------

static Symbol finish(Symbol s, const char *restrict desc) {
  // Suppress unused param warning
  (void) desc;
  dbg("finish: %s\n", desc);
  return s;
}

static Symbol finish_if_valid(Symbol s, const char *restrict desc) {
  if (valid(s)) return finish(s, desc);
  else return FAIL;
}

static Symbol finish_marked(Symbol s, const char *restrict desc) {
  (void) desc;
  MARK(desc);
  return s;
}

static Symbol update_state(const char *restrict desc) {
  return finish(UPDATE, desc);
}

// --------------------------------------------------------------------------------------------------------
// Lookahead
// --------------------------------------------------------------------------------------------------------

static void advance_over(uint32_t n) {
  for (uint32_t i = state->lookahead->len; i <= state->lookahead->offset + n; i++) S_ADVANCE;
}

static void advance_before(uint32_t n) {
  if (n > 0) advance_over(n - 1);
}

/**
 * Unsafe insofar as that it does not advance if the index points outside of the lookahead buffer.
 * This may happen in regular operation when a tool like `seq` attempts to look beyond EOF.
 */
static int32_t unsafe_peek_abs(uint32_t n) {
  return
    n < state->lookahead->len ?
    state->lookahead->data[n] :
    0;
}

static int32_t unsafe_peek(uint32_t n) {
  return unsafe_peek_abs(state->lookahead->offset + n);
}

static int32_t peek(uint32_t n) {
  if (state->lookahead->offset + n < state->lookahead->len) return unsafe_peek(n);
  else {
    advance_before(n);
    return PEEK;
  }
}

static int32_t peek0() {
  return peek(0);
}

static int32_t peek1() {
  return peek(1);
}

static int32_t peek2() {
  return peek(2);
}

static bool char_at(uint32_t n, int32_t c) {
  return peek(n) == c;
}

static bool char0(int32_t c) {
  return char_at(0, c);
}

static bool char1(int32_t c) {
  return char_at(1, c);
}

static bool char2(int32_t c) {
  return char_at(2, c);
}

static int32_t peek_after(const char *restrict s) {
  return peek((uint32_t) strlen(s));
}

static void reset_lookahead_to(uint32_t col) {
  state->lookahead->offset = col;
}

static void reset_lookahead() {
  reset_lookahead_to(state->lookahead->len);
}

static bool no_lookahead() {
  return state->lookahead->len == 0;
}

static uint32_t start_column() {
  return column() - state->lookahead->len;
}

static uint32_t advance_while(bool (*pred)(int32_t), uint32_t i) {
  while (pred(peek(i))) { i++; }
  return i;
}

// --------------------------------------------------------------------------------------------------------
// Context manipulation and conditions
// --------------------------------------------------------------------------------------------------------

static bool has_contexts() { return contexts->len != 0; }

static void push_context(ContextSort sort, uint32_t indent) {
  dbg("push: %s %d\n", context_names[sort], indent);
  Context ctx = (Context) {.sort = sort, .indent = indent};
  VEC_PUSH(contexts, ctx);
}

/**
 * Remove an indentation context from the stack.
 */
static void pop() {
  if (has_contexts()) {
    dbg("pop: %s\n", context_names[VEC_BACK(contexts).sort]);
    VEC_POP(contexts);
  }
}

static ContextSort current_context() {
  return has_contexts() ? VEC_BACK(contexts).sort : NoContext;
}

static bool is_layout_context() {
  return current_context() < Braces;
}

static bool is_semicolon_context() {
  return current_context() < MultiWayIfLayout;
}

static uint32_t current_indent() {
  for (int32_t i = (int32_t) contexts->len - 1; i >= 0; i--) {
    Context *cur = contexts->data + i;
    if (cur->sort < Braces) return cur->indent;
  }
  return 0;
}

static bool indent_less(uint32_t indent) {
  return is_layout_context() && indent < current_indent();
}

static bool indent_lesseq(uint32_t indent) {
  return is_layout_context() && indent <= current_indent();
}

static bool top_layout() {
  return contexts->len == 1;
}

static bool in_module_header() {
  return current_context() == ModuleHeader;
}

static Symbol context_end_sym(ContextSort s) {
  switch (s) {
    case TExp:
      return END_TEXP;
    case Braces:
      return END_BRACE;
    default:
      return s < Braces ? END : FAIL;
  }
}

// --------------------------------------------------------------------------------------------------------
// Character and lookahead conditions
// --------------------------------------------------------------------------------------------------------

#define NEWLINE_CASES \
  case '\n': \
  case '\r': \
  case '\f'


static bool is_newline(int32_t c) {
  switch (c) {
    NEWLINE_CASES:
      return true;
    default:
      return false;
  }
}

static bool varid_start_char(const int32_t c) { return c == '_' || is_varid_start_char(c); }

static bool is_id_char(const int32_t c) {
  return c == '_' || c == '\'' || is_identifier_char(c);
}

static bool is_inner_id_char(const int32_t c) {
  return is_id_char(c) || c == '#';
}

static bool quoter_char(const int32_t c) { return is_id_char(c) || c == '.'; }

static bool reserved_symbolic(const int32_t c) {
  switch (c) {
    case '(':
    case ')':
    case ',':
    case ';':
    case '[':
    case ']':
    case '`':
    case '{':
    case '}':
    case '"':
    case '\'':
    case '_':
      return true;
    default: return false;
  }
}

static bool symop_char(const int32_t c) {
  return is_symop_char(c) && !reserved_symbolic(c);
}

/**
 * This consumes the entire symop, since the field denotes the length of the string and therefore the last (failing)
 * peek is _beyond_ the end, consuming the last valid char.
 */
static uint32_t symop_lookahead() {
  if (env->symop == 0) { env->symop = advance_while(symop_char, 0); }
  return env->symop;
}

/**
 * The parser calls `scan` with all symbols declared as valid directly after it encountered an error.
 * The symbol `FAIL` is not used in the grammar, so it can only be valid in this error case.
 */
static bool all_syms(const bool *syms) {
  return syms[FAIL];
}

// --------------------------------------------------------------------------------------------------------
// Debug printing
// --------------------------------------------------------------------------------------------------------

#if DEBUG

static void push_parse_buffer_line() {
  ParseLine new_line = {.len = 0};
  VEC_GROW(&new_line, 1);
  VEC_PUSH(&state->parse, new_line);
}

static ParseLine *ensure_parse_buffer() {
  ParseLines *buffer = &state->parse;
  if (buffer->len == 0) push_parse_buffer_line();
  if (is_newline(PEEK)) push_parse_buffer_line();
  return buffer->data + buffer->len - 1;
}

static void append_parse_buffer() {
  ParseLine *current_line = ensure_parse_buffer();
  if (is_newline(PEEK)) {
    env->debug.marked_line++;
    env->debug.start_line++;
  }
  else if (column() >= current_line->len) VEC_PUSH(current_line, PEEK);
}

static void fill_parse_buffer() {
  env->debug.end_col = column();
  while (!(is_newline(PEEK) || is_eof())) S_ADVANCE;
}

static bool seq(const char *restrict s);

static void push_comment_to_parse_buffer() {
  if (seq("{-")) {
    dbg("pushing comment\n");
    uint16_t level = 1;
    while (level > 0) {
      reset_lookahead();
      if (seq("{-")) level++;
      else if (seq("-}")) level--;
      else advance_over(0);
    }
  }
}

static void print_lookahead() {
  dbg("lookahead: %.*ls\n", state->lookahead->len, state->lookahead->data);
}

static const char * space = "<space>";
static const char * newline_char = "\\n";

static const char * show_char(int32_t c) {
  switch (c) {
    NEWLINE_CASES:
      return newline_char;
    case ' ':
    case '\t':
    case '\v':
      return space;
    default:
      return NULL;
  }
}

static void print_lookahead_chars_from(uint32_t start) {
  if (start < state->lookahead->len) {
    dbg("lookahead from %d: ", start);
    for (; start < state->lookahead->len; start++) {
      int32_t c = state->lookahead->data[start];
      const char * s = show_char(c);
      if (s == NULL) dbg("%lc", c);
      else dbg("%s", s);
    }
    dbg("\n");
  }
  else
    dbg("print_lookahead_chars_from: Too large (%d / %d)", start, state->lookahead->len);
}

static void debug_contexts() {
  if (contexts->len == 0) dbg("empty");
  bool empty = true;
  for (size_t i = 0; i < contexts->len; i++) {
    if (!empty) dbg("-");
    Context ctx = contexts->data[i];
    if (ctx.sort == ModuleHeader) dbg("pre");
    else if (ctx.sort == Braces) dbg("brace");
    else if (ctx.sort == TExp) dbg("texp");
    else {
      if (ctx.sort == DoLayout) dbg("do ");
      else if (ctx.sort == LetLayout) dbg("let ");
      else if (ctx.sort == CaseLayout) dbg("case ");
      else if (ctx.sort == MultiWayIfLayout) dbg("if ");
      else if (ctx.sort == QuoteLayout) dbg("quote ");
      dbg("%d", ctx.indent);
    }
    empty = false;
  }
}

void debug_newline() {
  switch (newline->state) {
    case NInactive:
      dbg("no");
      break;
    case NInit:
      dbg("init");
      break;
    case NProcess:
      dbg("process");
      break;
    case NResume:
      dbg("resume");
      break;
  }
  if (newline->state != NInactive) dbg(" %d %s", newline->indent, token_names[newline->end]);
  if (newline->eof) dbg(" [eof]");
  if (newline->no_semi) dbg(" [no_semi]");
  if (newline->skip_semi) dbg(" [skip_semi]");
  if (newline->unsafe) dbg(" [unsafe]");
}

/**
 * Produce a comma-separated string of valid symbols.
 */
static void debug_valid(const bool *syms) {
  if (all_syms(syms)) {
    dbg("all");
    return;
  }
  bool fst = true;
  dbg("\"");
  for (Symbol i = SEMICOLON; i <= FAIL; i++) {
    if (syms[i]) {
      if (!fst) dbg(",");
      dbg("%s", sym_names[i]);
      fst = false;
    }
  }
  dbg("\"");
}

static bool debug_init() {
  setlocale(LC_ALL, "C.UTF-8");
  dbg("\n");
  dbg("Env { syms = ");
  debug_valid(env->symbols);
  dbg(", contexts = ");
  debug_contexts();
  dbg(", newline = ");
  debug_newline();
  if (state->disabled) dbg(", disabled");
  dbg(" }\n");
  return false;
}

void sgr(const char *restrict code) {
  dbg("\x1b[%sm", code);
}

void color(unsigned c) {
  char code[3];
  sprintf(code, "3%d", c);
  sgr(code);
}

void palette() {
  color(4);
  dbg("before");
  color(2);
  dbg("  marked");
  color(3);
  dbg("  advanced");
  color(5);
  dbg("  lookahead");
  sgr("");
  dbg("\n");
}

static bool debug_parse_metadata = false;

static void dump_parse_metadata() {
  Debug *debug = &env->debug;
  dbg(
      "lines: %d | start_line: %d | start_col: %d | marked_line: %d | marked: %d | end_col: %d | persist lines: %d\n",
      state->parse.len,
      debug->start_line,
      debug->start_col,
      debug->marked_line,
      debug->marked,
      debug->end_col,
      state->parse.len - debug->marked_line
  );
}

/**
 * Note: We're printing individual characters here instead of using a format with precision like `%.*ls` and slicing
 * the buffer, because:
 * - The buffer contains wide characters, but `fprintf` counts bytes
 * - `fwprintf` counts wide characters, but can't be interleaved with `fprintf`, so we'd have to use that function, and
 *   therefore wide literals, everywhere, which is tedious
 */
void debug_parse() {
  Debug *debug = &env->debug;
  ParseLines *buffer = &state->parse;
  uint32_t lines = buffer->len;
  dbg("-----------------------\n");
  // For investigating mistakes in the debugging code.
  if (debug_parse_metadata) dump_parse_metadata();
  if (lines > 0) {
    color(4);
    for (uint32_t i = 0; i < lines; i++) {
      ParseLine *line = buffer->data + i;
      int32_t *buf = line->data;
      if (line->data == 0) break;
      uint32_t pos = 0;

      if (debug->start_line == lines - 1 - i) {
        while (pos < debug->start_col) dbg("%lc", buf[pos++]);
        color(2);
      }

      if (debug->marked >= 0 && debug->marked_line == lines - 1 - i) {
        while ((int) pos < debug->marked) dbg("%lc", buf[pos++]);
        color(3);
      }

      if (i == lines - 1) {
        while (pos < debug->end_col) dbg("%lc", buf[pos++]);
        color(5);
      }

      while (pos < line->len) dbg("%lc", buf[pos++]);

      dbg("\n");
    }
    sgr("");
  }
  dbg("-----------------------\n");
}

static unsigned serialize_parse_lines(char *cursor, ParseLines *parse, unsigned to_copy) {
  for (unsigned i = 0; i < parse->len; i++) {
    ParseLine *line = &parse->data[i];
    unsigned line_size = line->len * sizeof(uint32_t);
    to_copy += line_size + sizeof(uint32_t);
    if (to_copy > TREE_SITTER_SERIALIZATION_BUFFER_SIZE) return 0;
    *((uint32_t *) cursor) = line->len;
    cursor += sizeof(line->len);
    memcpy(cursor, line->data, line_size);
    cursor += line_size;
  }
  return to_copy;
}

static void deserialize_parse_lines(const char *cursor, ParseLines *parse, uint32_t len) {
  // Ensure ParseLines has room for at _least_ as many lines as the new state
  VEC_GROW(parse, len);
  for (unsigned i = 0; i < len; i++) {
    // If the new state has more lines, properly initialize them.
    if (i >= parse->len) { VEC_PUSH(parse, (ParseLine) {.len = 0}); }
    ParseLine *line = &parse->data[i];
    uint32_t line_len = *((uint32_t *) cursor);
    cursor += sizeof(uint32_t);
    VEC_GROW(line, line_len);
    line->len = line_len;
    unsigned line_size = line->len * sizeof(uint32_t);
    memcpy(line->data, cursor, line_size);
    cursor += line_size;
  }
  // Free the excessive lines in the previous since we can't check in the next round whether there was a line in
  // a slot before and reuse the pointer.
  // This only happens when we didn't push any lines above, which would reset parse->len to len.
  for (unsigned i = parse->len; i > len; i--) { VEC_FREE(&parse->data[i - 1]); }
  // Truncate ParseLines in case the new state has fewer lines
  parse->len = len;
}

void debug_finish(Symbol result) {
  dbg("result: ");
  if (result) dbg("%s, ", sym_names[result]);
  else dbg("<skipped>, ");
  if (env->debug.marked == -1) dbg("%d", column());
  else dbg("%s@%d", env->debug.marked_by, env->debug.marked);
  dbg("\n\n");
  if (state->disabled) {
    if (result) {
      push_comment_to_parse_buffer();
      debug_parse();
    }
  }
  else {
    fill_parse_buffer();
    debug_parse();
    state->parse.len -= env->debug.marked_line;
  }
}

#endif

// --------------------------------------------------------------------------------------------------------
// Lookahead
// --------------------------------------------------------------------------------------------------------

static bool seq_from(const char *restrict s, uint32_t start) {
  uint32_t len = (uint32_t) strlen(s);
  for (uint32_t i = 0; i < len; i++) {
    int32_t c = s[i];
    int32_t c2 = peek(start + i);
    if (c != c2) return false;
  }
  peek(start + len);
  return true;
}

static bool seq(const char *restrict s) {
  return seq_from(s, 0);
}

/**
 * Consume the body of a cpp directive or comment.
 */
static void take_line() {
  for (;;) {
    while (not_eof() && !is_newline(PEEK) && PEEK != '\\') S_ADVANCE;
    if (PEEK == '\\') {
      S_ADVANCE;
      S_ADVANCE;
    }
    else return;
  }
}

/**
 * Skip the lexer until the following character is neither space nor tab.
 */
static bool skip_space() {
  if (!is_space_char(PEEK)) return false;
  S_SKIP;
  while(is_space_char(PEEK)) S_SKIP;
  return true;
}

static bool skip_newlines() {
  if (!is_newline(PEEK)) return false;
  S_SKIP;
  while(is_newline(PEEK)) S_SKIP;
  return true;
}

typedef enum {
  NoSpace,
  Indented,
  BOL,
} Space;

static Space skip_space_and_newline() {
  Space space = NoSpace;
  while (true) {
    if (skip_space()) space = Indented;
    else if (skip_newlines()) space = BOL;
    else return space;
  };
}

static uint32_t take_space_from(uint32_t start) {
  return advance_while(is_space_char, start);
}

/**
 * Ensure that the character after a keyword like `module` is not a character that would change its meaning to be an
 * identifier.
 */
static bool token_end(int32_t c) { return !is_inner_id_char(c); }

/**
 * Require that the argument string follows the given start position and is followed by a non-id char.
 * See `seq`.
 */
static bool token_from(const char *restrict s, uint32_t start) {
  return seq_from(s, start) && token_end(peek(start + (uint32_t) strlen(s)));
}

/**
 * `token` at the current offset.
 */
static bool token(const char *restrict s) {
  return seq(s) && token_end(peek_after(s));
}

static bool any_token_from(size_t n, const char * tokens[n], uint32_t start) {
  for (size_t i = 0; i < n; i++) {
    if (token_from(tokens[i], start)) return true;
  }
  return false;
}

static bool uninitialized() { return !has_contexts(); }

static bool after_error() { return all_syms(env->symbols); }

// --------------------------------------------------------------------------------------------------------
// Lookahead: CPP
// --------------------------------------------------------------------------------------------------------

typedef enum {
  CppNothing,
  CppStart,
  CppElse,
  CppEnd,
  CppOther,
} CppDirective;

static const char *cpp_tokens_start[3] = {
  "if",
  "ifdef",
  "ifndef",
};

static bool cpp_cond_start(uint32_t start) {
  return any_token_from(3, cpp_tokens_start, start);
}

static const char *cpp_tokens_else[4] = {
  "else",
  "elif",
  "elifdef",
  "elifndef",
};

static bool cpp_cond_else(uint32_t start) {
  return any_token_from(4, cpp_tokens_else, start);
}

static bool cpp_cond_end(uint32_t start) { return token_from("endif", start); }

static const char *cpp_tokens_other[7] = {
  "define",
  "undef",
  "include",
  "pragma",
  "error",
  "warning",
  "line",
};

static bool cpp_directive_other(uint32_t start) {
  return
    any_token_from(7, cpp_tokens_other, start)
    ||
    // A hash followed by nothing but whitespace is CPP.
    // If non-whitespace follows whitespace, it is a parse error, unless we're in a brace layout; then it is a varsym.
    // Complete overkill to parse this, but eh!
    is_newline(peek(start))
    ||
    // shebang for scripts
    (char1('!') && uninitialized())
    ;
}

static CppDirective cpp_directive() {
  if (!char0('#')) return CppNothing;
  uint32_t start = take_space_from(1);
  if (cpp_cond_start(start)) return CppStart;
  else if (cpp_cond_else(start)) return CppElse;
  else if (cpp_cond_end(start)) return CppEnd;
  else if (cpp_directive_other(start)) return CppOther;
  else return CppNothing;
}

// --------------------------------------------------------------------------------------------------------
// Starting layouts
// --------------------------------------------------------------------------------------------------------

static Symbol start_brace() {
  if (valid(START_BRACE)) {
    push_context(Braces, 0);
    return finish(START_BRACE, "start_brace");
  }
  return FAIL;
}

/**
 * Not failing when brace is valid without being in a brace context – maybe it'll help with error recovery.
 */
static Symbol end_brace() {
  if (valid(END_BRACE) && current_context() == Braces) {
    pop();
    return finish(END_BRACE, "end_brace");
  }
  return FAIL;
}

// Order of precedence for layouts
static Symbol valid_layout_start_sym() {
  for (Symbol i = START; i < END; i++) {
    if (valid(i)) return i;
  }
  return FAIL;
}

static ContextSort layout_sort(Symbol s) {
  switch (s) {
    case START_DO:
      return DoLayout;
    case START_CASE:
      return CaseLayout;
    case START_IF:
      return MultiWayIfLayout;
    case START_LET:
      return LetLayout;
    case START_QUOTE:
      return QuoteLayout;
    default:
      return DeclLayout;
  }
}

typedef struct {
  Symbol sym;
  ContextSort sort;
} StartLayout;

static StartLayout valid_layout_start(Lexed next) {
  StartLayout start = {.sym = valid_layout_start_sym(), .sort = NoContext};
  if (uninitialized() || start.sym == FAIL) return start;
  ContextSort sort = layout_sort(start.sym);
  switch (next) {
    case LBar:
      break;
    case LBraceOpen:
      if (newline_active()) return start;
      sort = Braces;
      break;
    // `if/then/else` is always valid at the same position as MWI, so if we unconditionally start a layout, it will
    // never be possible to parse the former.
    default:
      if (sort == MultiWayIfLayout) return start;
      break;
  }
  start.sort = sort;
  return start;
}

/**
 * If the current context is a brace layout, any indent is legal for a new layout.
 * Otherwise, compare with the indent of the current context.
 * Since starting layouts is allowed in tuple expressions, we look at the last real indent, skipping over `TExp`s, using
 * 0 if none exists (which should never be the case).
 *
 * According to the docs for `NondecreasingIndentation`, the rule is that a nested context may start at the same column
 * _if the enclosing context is a do expression_.
 * From experimental evidence, it appears though that this is the other way round – a do expression within, say, a case
 * alt can start at the same level as the case layout.
 */
static bool indent_can_start_layout(ContextSort sort, uint32_t indent) {
  if (current_context() == Braces) return true;
  uint32_t cur = current_indent();
  return (indent > cur || (indent == cur && sort == DoLayout));
}

static Symbol start_layout(const StartLayout start, uint32_t indent, const char * restrict desc) {
  bool header = in_module_header();
  if (header || start.sort == Braces || indent_can_start_layout(start.sort, indent)) {
    if (header) pop();
    if (start.sort == Braces) MARK("start_layout brace");
    push_context(start.sort, indent);
    return finish(start.sym, desc);
  }
  return FAIL;
}

/**
 * The indent of a layout started at an interior token can only be determined by calling `get_column`.
 * This is an expensive operation, but hopefully it is rare enough to not make a substantial dent.
 * Because we might have advanced beyond the layout's start position to check conditions, we need to subtract the length
 * of the lookahead buffer from the current column.
 * Whitespace is skipped, and not added to the buffer, so the resulting position is after whitespace.
 */
static Symbol start_layout_interior(Lexed next) {
  StartLayout start = valid_layout_start(next);
  if (start.sort == NoContext) return FAIL;
  return start_layout(start, start_column(), "interior");
}

/**
 * The indent of a layout started at the beginning of a line is determined by `newline_lookahead`, so this does not have
 * to compute it.
 */
static Symbol start_layout_newline() {
  StartLayout start = valid_layout_start(newline->end);
  if (start.sort == NoContext) return FAIL;
  Symbol result = start_layout(start, newline->indent, "newline");
  if (result != FAIL) newline->no_semi = true;
  return result;
}

static Symbol tuple_context() {
  if (valid(START_TEXP)) {
    push_context(TExp, 0);
    return finish(START_TEXP, "tuple_context");
  }
  else if (valid(END_TEXP) && current_context() == TExp) {
    pop();
    return finish(END_TEXP, "tuple_context");
  }
  else return FAIL;
}

// --------------------------------------------------------------------------------------------------------
// Ending layouts
// --------------------------------------------------------------------------------------------------------

/**
 * Separated this from `end_layout` because it caused some weird performance glitches.
 */
static Symbol end_layout_unchecked(const char *restrict desc) {
  pop();
  return finish(END, desc);
}

/**
 * If a layout end is valid at this position, pop a context and succeed with layout end.
 */
static Symbol end_layout(const char *restrict desc) {
  if (valid(END)) return end_layout_unchecked(desc);
  else return FAIL;
}

static Symbol end_layout_brace() {
  if (valid(END) && current_context() == Braces) {
    peek1();
    MARK("end_layout_brace");
    return end_layout_unchecked("brace");
  }
  else return FAIL;
}

/**
 * End a layout based on indent decrease.
 *
 * If the indent of the current line is smaller than the indent of the current layout, we end the layout in most cases.
 * Exceptions are:
 *
 * - Brace layouts
 * - The top-level layout, which should only be ended at the end of file.
 *   For leniency, we change the current indent to the smaller value.
 */
static Symbol end_layout_indent() {
  if (valid(END) && indent_less(newline->indent)) {
    if (top_layout()) {
      VEC_BACK(contexts).indent = newline->indent;
      return update_state("end top layout");
    }
    else {
      newline->skip_semi = false;
      return end_layout_unchecked("indent");
    }
  }
  return FAIL;
}

/**
 * An expression layout may be closed by an infix operator when it is not valid at that position:
 *
 * a :: IO Int
 * a = do a <- pure 5
 *        pure a
 *        >>= pure
 *
 * In this situation, the indent of the operator causes a semicolon to be generated, which leads to varsym being invalid
 * lookahead.
 * The layout is closed and the entire `do` block becomes the left operand of the `>>=`.
 * The same applies for `infix` id operators.
 *
 * It doesn't apply to multi-way if layouts, because those don't use semicolons.
 */
static Symbol end_layout_infix() {
  if (!valid(VARSYM) && !valid(CONSYM)) return end_layout("symop invalid");
  return FAIL;
}

/**
 * A case alt can have a `where` clause attached to it, so a case layout is ended by a `where` only if its indent is
 * equal to or smaller than the layout indent.
 *
 * A `do` or `if` cannot have a `where`, so they are always terminated.
 *
 * It would be tempting to leave it at that, but there can be multiple successive `where` clauses.
 * If a `case` is followed by two of them (greater indent), the first one would attach to the last alt.
 * The second one would have to close the `case` layout and attach to the next higher allowed place (e.g. the enclosing
 * function decl), but if its indent is greater, this cannot be detected here – it would just seem like a `where`
 * attaching to an alt, since we don't keep track of the number of `where`s encountered (and we couldn't, since we're
 * dealing with layouts, not case alts).
 *
 * By tracking the validity of `where` symbols, we can simplify the condition for `do` and `if`: End any layout when
 * `where` is parsed, but invalid.
 */
static Symbol end_layout_where() {
  if (valid(END) && !valid(WHERE) && is_layout_context()) return end_layout("where");
  return FAIL;
}

/**
 * Ending layouts with `in` heavily relies on parse errors in GHC, so this is difficult.
 * The heuristic here is that if `in` is not valid (i.e. a parse error), we pop any layout.
 *
 * Take the example of some inline layouts in a let decl:
 * `let a = case a of a -> do a in a`
 * The `do` and `case` layouts have valid `END` symbols at the `in`, but `in` itself is not valid as long as the `case`
 * hasn't reduced, so we pop until we find `IN`.
 *
 * This isn't enough though, since `let` also opened a layout that ends here, so we have to test for that explicitly.
 *
 * Note that this doesn't allow the `in` of a nested `let` to close the outer `let`, since the `END` for that isn't
 * valid before the inner `let` has reduced.
 */
static Symbol end_layout_in() {
  if (valid(END) && (!valid(IN) || current_context() == LetLayout)) return end_layout("in");
  return FAIL;
}

/**
 * For GADT constructor layouts.
 */
static Symbol end_layout_deriving() {
  if (valid(END) && !valid(DERIVING) && !top_layout() && current_context() == DeclLayout)
    return end_layout("deriving");
  return FAIL;
}

/**
 * Return `true` if there is a `TExp` context on the stack and only layouts above it.
 */
static bool layouts_in_texp() {
  if (is_layout_context() && (contexts->len > 1)) {
    for (int32_t i = (int32_t) contexts->len - 2; i >= 0; i--) {
      Context *cur = contexts->data + i;
      if (cur->sort == TExp || cur->sort == Braces) return true;
      else if (cur->sort > Braces) break;
    }
  }
  return false;
}

/**
 * Expression layouts can be closed by commas, vertical bars and closing brackets and parens when they are elements in a
 * list or tuple-like construct:
 *
 * (case a of a -> a, do a; a, if | a -> a | a -> a)
 * [case a of a -> a | a <- a]
 *
 * This pattern also extends to some other this like guards:
 *
 * > a = case a of
 * >   a | let a = a -> a
 *
 * Here the arrow ends the let layout.
 */
static Symbol token_end_layout_texp() {
  return (valid(END) && layouts_in_texp()) ? end_layout("texp") : FAIL;
}

static Symbol force_end_context() {
  for (int32_t i = (int32_t) contexts->len - 1; i >= 0; i--) {
    ContextSort ctx = contexts->data[i].sort;
    Symbol s = context_end_sym(ctx);
    pop();
    if (s != FAIL && valid(s)) return finish(s, "force_end_context");
  }
  return FAIL;
}

// --------------------------------------------------------------------------------------------------------
// Operators
// --------------------------------------------------------------------------------------------------------

static bool opening_token(uint32_t i) {
  int32_t c = peek(i);
  switch (c) {
    case 0x27e6: // ⟦
    case 0x2987: // ⦇
    case '(':
    case '[':
    case '"':
      return true;
    case '{':
      return peek(i + 1) != '-';
    default:
      return is_id_char(c);
  }
}

/**
 * Test for reserved operators of two characters.
 */
static bool valid_symop_two_chars(int32_t first_char, int32_t second_char) {
  switch (first_char) {
    case '-':
      return second_char != '-' && second_char != '>';
    case '=':
      return second_char != '>';
    case '<':
      return second_char != '-';
    case '.':
      return second_char != '.';
    case ':':
      return second_char != ':';
    case '#':
      return second_char != '#' && second_char != '|';
    default:
      return true;
  }
}

static Lexed lex_prefix(Lexed t) {
  return opening_token(1) ? t : LSymop;
}

static Lexed lex_splice(int32_t c) {
  return varid_start_char(c) || c == '(' ? LDollar : LSymop;
}

/**
 * Lex special occurrences of symbolic operator characters, or declare a valid operator.
 *
 * For the dot:
 *
 * - Two dots: `..`: Only used for arithmetic sequences (`[a..10]`).
 *   These conflict with record field projection (`[a.b, c]`) and infix operators (`[a..+b]`), and it's too hard to
 *   disambiguate them without this special rule.
 *
 * - Tight dot `a.b.c`: A regular tight op, but it has to get a separate symbol from qualified module dots since those
 *   can be followed by symops.
 *
 * - Prefix dot `(.a)`: A regular prefix op, for record dot field selectors.
 *
 * - Qualified dot `A.B.c`, `A.B.C`, `A.B.+`: Used primarily for qualified modules, but needs to be accepted for field
 *   selectors as well due to ambiguity.
 *   This is not a regular tight op since it needs to allow symops and conid.
 */

static Lexed lex_symop() {
  uint32_t len = symop_lookahead();
  if (len == 0) return LNothing;
  int32_t c1 = unsafe_peek(0);
  if (len == 1) {
    switch (c1) {
      case '?':
        return varid_start_char(peek1()) ? LNothing : LSymop;
      case '#':
        return char1(')') ? LUnboxedClose : LHash;
      case '|':
        return char1(']') ? LQuoteClose : LBar;
      case '!':
        return lex_prefix(LBang);
      case '~':
        return lex_prefix(LTilde);
      case '@':
        return lex_prefix(LAt);
      case '%':
        return lex_prefix(LPercent);
      case '$':
        return lex_splice(peek1());
      case '.':
        if (is_id_char(peek1())) return LDotId;
        else if (opening_token(1)) return LDotOpen;
        else return LSymop;
      case 0x2192: // →
        return LArrow;
      case '=':
      case 0x27e7: // ⟧
      case 0x2988: // ⦈
        return LTexpCloser;
      case '\\':
      case '*':
      case '-':
      case 0x2190: // ←
      case 0x21d2: // ⇒
      case 0x2200: // ∀
      case 0x2237: // ∷
      case 0x22b8: // ⊸
      case 0x2605: // ★
      case 0x27e6: // ⟦
      case 0x2919: // ⤙
      case 0x291a: // ⤚
      case 0x291b: // ⤛
      case 0x291c: // ⤜
      case 0x2987: // ⦇
        return LNothing;
    }
  }
  else if (len == 2) {
    if (seq("->")) return LArrow;
    int32_t c2 = unsafe_peek(1);
    switch (c1) {
      case '$':
        if (c2 == '$') return lex_splice(peek2());
        break;
      case '|':
        if (c2 == '|' && char2(']')) return LQuoteClose;
        break;
      case '.':
        if (c2 == '.') return LDotDot;
        else return LDotSymop;
        break;
      default:
        if (!valid_symop_two_chars(c1, c2)) return LNothing;
        break;
    }
  }
  else switch (c1) {
    case '-':
      if (seq("->.")) return LNothing;
      break;
    case '.':
      return LDotSymop;
  }
  return LSymop;
}

static Symbol finish_symop(Symbol s) {
  if (valid(s)) {
    advance_before(symop_lookahead());
    return finish_marked(s, "symop");
  }
  return FAIL;
}

static Symbol tight_op(bool whitespace, Symbol s) {
  if (!whitespace) return finish_if_valid(s, "tight_op");
  else return FAIL;
}

static Symbol prefix_or_varsym(bool whitespace, Symbol s) {
  if (whitespace) SEQ(finish_if_valid(s, "prefix_or_varsym"));
  return finish_symop(VARSYM);
}

static Symbol tight_or_varsym(bool whitespace, Symbol s) {
  SEQ(tight_op(whitespace, s));
  return finish_symop(VARSYM);
}

static Symbol infix_or_varsym(bool whitespace, Symbol prefix, Symbol tight) {
  SEQ(finish_if_valid(whitespace ? prefix : tight, "infix_or_varsym"));
  return finish_symop(VARSYM);
}

// --------------------------------------------------------------------------------------------------------
// Token lookahead
// --------------------------------------------------------------------------------------------------------

/**
 * Detect the start of a quasiquote: An opening bracket followed by an optional varid and a vertical bar, all without
 * whitespace in between.
 */
static bool is_qq_start() {
  uint32_t end = advance_while(quoter_char, 1);
  return char_at(end, '|');
}

static Lexed try_end_token(const char * restrict target, Lexed match) {
  if (token(target)) return match;
  else return LNothing;
}

static bool only_minus() {
  uint32_t i = 2;
  while (peek(i) == '-') i++;
  return !symop_char(peek(i));
}

static bool line_comment_herald() {
  return seq("--") && only_minus();
}

static Lexed lex_cpp() {
  switch(cpp_directive()) {
    case CppElse:
      return LCppElse;
    case CppNothing:
      return LNothing;
    default:
      return LCpp;
  }
}

static Lexed lex_extras(bool bol) {
  switch (peek0()) {
    case '{':
      if (char1('-')) return char2('#') ? LPragma : LBlockComment;
      break;
    case '#':
      if (bol) return lex_cpp();
      break;
    case '-':
      if (line_comment_herald()) return LLineComment;
      break;
    default:
      break;
  }
  return LNothing;
}

static Lexed lex(bool bol) {
  SEQT(lex_extras(bol));
  if (symop_char(peek0())) SEQT(lex_symop());
  else switch (peek0()) {
    case 'w':
      return try_end_token("where", LWhere);
    case 'i':
      return try_end_token("in", LIn);
    case 't':
      return try_end_token("then", LThen);
    case 'e':
      return try_end_token("else", LElse);
    case 'd':
      return try_end_token("deriving", LDeriving);
    case 'm':
      if ((uninitialized() || in_module_header()) && token("module")) return LModule;
      break;
    case '{':
      return LBraceOpen;
    case '}':
      return LBraceClose;
    case ';':
      return LSemi;
    case '`':
      return LTick;
    case '[':
      if (valid(QQ_START) && is_qq_start()) return LBracketOpen;
      break;
    case ']':
    case ')':
    case ',':
      return LTexpCloser;
    default:
      break;
  }
  return LNothing;
}

// --------------------------------------------------------------------------------------------------------
// Actions that consume arbitrary text - cpp, comments, quasiquote
// --------------------------------------------------------------------------------------------------------

/**
 * This tests for `#endif` directly after taking a line, so it only matches it at the first column.
 * Int finishes right before the `#endif`, so that pragma is parsed by `cpp_consume` in the next round.
 */
static Symbol cpp_else(bool emit) {
  uint32_t nesting = 1;
  do {
    take_line();
    if (emit) MARK("cpp_else");
    S_ADVANCE;
    reset_lookahead();
    switch (cpp_directive()) {
      case CppStart:
        nesting++;
        break;
      case CppEnd:
        nesting--;
        break;
      default:
        break;
    }
  }
  while (not_eof() && nesting > 0);
  if (emit) return finish(CPP, "cpp-else");
  else return FAIL;
}

static Symbol cpp_line() {
  take_line();
  return finish_marked(CPP, "cpp");
}

static Symbol comment_type() {
  uint32_t i = 2;
  while (peek(i) == '-') i++;
  while (not_eof()) {
    int32_t c = peek(i++);
    if (c == '|' || c == '^') return HADDOCK;
    else if (!is_space_char(c)) break;
  }
  return COMMENT;
}

static Symbol inline_comment() {
  Symbol sym = comment_type();
  do {
    take_line();
    MARK("inline comment");
    S_ADVANCE;
    reset_lookahead();
  } while (line_comment_herald());
  return sym;
}

static uint32_t consume_block_comment(uint32_t col) {
  uint32_t level = 0;
  for (;;) {
    if (is_eof()) return col;
    col++;
    switch (PEEK) {
      case '{':
        S_ADVANCE;
        if (PEEK == '-') {
          S_ADVANCE;
          col++;
          level++;
        }
        break;
      case '-':
        S_ADVANCE;
        if (PEEK == '}') {
          S_ADVANCE;
          col++;
          if (level == 0) return col;
          level--;
        }
        break;
      NEWLINE_CASES:
        S_ADVANCE;
        col = 0;
        break;
      default:
        S_ADVANCE;
        break;
    }
  }
}

/**
 * Since {- -} comments can be nested arbitrarily, this has to keep track of how many have been openend, so that the
 * outermost comment isn't closed prematurely.
 */
static Symbol block_comment() {
  Symbol sym = comment_type();
  consume_block_comment(state->lookahead->len);
  return finish_marked(sym, "block_comment");
}

static bool consume_pragma() {
  if (seq("{-#")) {
    while (!seq("#-}") && not_eof()) {
      reset_lookahead();
      advance_over(0);
    }
    MARK("pragma");
    return true;
  }
  return false;
}

static Symbol pragma() {
  if (consume_pragma()) {
    if (newline->state != NInactive) newline->state = NResume;
    return finish(PRAGMA, "newline");
  }
  return FAIL;
}

static Symbol qq_body() {
  for (;;) {
    if (is_eof()) return finish(QQ_BODY, "qq_body");
    else if (PEEK == 0x27e7) {
      return finish_marked(QQ_BODY, "qq_body");
    }
    else if (PEEK == '|') {
      MARK("qq_body");
      S_ADVANCE;
      if (PEEK == ']') {
        return finish(QQ_BODY, "qq_body");
      }
    } else S_ADVANCE;
  }
}

// --------------------------------------------------------------------------------------------------------
// Semicolon
// --------------------------------------------------------------------------------------------------------

/**
 * When encountering explicit semicolons, we want to ensure that a subsequent newline doesn't trigger a layout
 * semicolon, so we set `skip_semi`.
 * If the next symbol is not a newline (and not another semicolon), the scanner will immediate end up in this function
 * again, matching the second branch, in which we unset the flag to avoid a mid-line semicolon from influencing an
 * unrelated newline.
 *
 * Take this example:
 *
 * > a = 1;;
 * > b = 2
 * > ;;c = 3
 *
 * At the first semicolon, the first branch is chosen and SEMICOLON is valid, so the flag is set.
 * The scanner will be called again immediately without advancing, so we hit the first branch again.
 * SEMICOLON is valid, but since the flag is set we fall through and defer to internal lexing.
 * The grammar advances into `semi` (in `util.js`), which causes SEMICOLON to become invalid.
 * The scanner is executed before the second semicolon, where we hit the first branch again and fall through again, this
 * time also because SEMICOLON is invalid.
 *
 * In the next scan, the newline branch is taken in `scan`, so this function is not executed again.
 * Newline lookahead finds the next line to begin at column 0, which would usually trigger a semicolon, but that is
 * skipped because the `skip_semi` was set, so the scan only skips whitespace and resets the newline state.
 * The following scan ends up in this function again, but since the next character isn't a semicolon and the flag was
 * reset at the end of the newline loop, it just falls through and continues parsing the line until the next newline.
 *
 * Newline lookahead now encounters the third semicolon on the next line, which also sets `skip_semi` – not from this
 * function, but directly in `newline_lookahead`.
 * The scan finishes with a state update, after which the two semicolons are parsed.
 * For both, the run ends up here but since SEMICOLON is not valid, it just falls through.
 */
static Symbol explicit_semicolon() {
  if (valid(SEMICOLON) && !newline->skip_semi) {
    newline->skip_semi = true;
    return update_state("explicit semicolon enable");
  }
  return FAIL;
}

static Symbol resolve_semicolon(Lexed next) {
  if (newline->skip_semi) {
    switch(next) {
      case LLineComment:
      case LBlockComment:
      case LSemi:
        break;
      default:
        newline->skip_semi = false;
        return update_state("explicit semicolon disable");
    }
  }
  return FAIL;
}

// --------------------------------------------------------------------------------------------------------
// High-level `Lexed` dispatch
// --------------------------------------------------------------------------------------------------------

/**
 * This is called by `newline_post` before marking, so they must not fail after advancing.
 */
static Symbol process_token_safe(Lexed next) {
  switch (next) {
    case LWhere:
      return end_layout_where();
    case LIn:
      return end_layout_in();
    case LThen:
    case LElse:
      return end_layout("then/else");
    case LDeriving:
      return end_layout_deriving();
    case LBar:
      if (!valid(BAR)) return end_layout("bar");
      break;
    case LPragma:
      return pragma();
    case LBlockComment:
      return block_comment();
    case LLineComment:
      return inline_comment();
    case LCppElse:
      return cpp_else(true);
    case LCpp:
      return cpp_line();
    case LSymop:
    case LTick:
    case LHash:
      return end_layout_infix();
    case LUnboxedClose:
      // Unboxed closing bracket can still be an operator, like `(#)` or `(5 #)`.
      // Can't think of a situation in which this could erroneously close a layout.
      SEQ(token_end_layout_texp());
      return end_layout_infix();
    case LArrow:
      if (!valid(ARROW)) return token_end_layout_texp();
      break;
    case LTexpCloser:
      return token_end_layout_texp();
    case LQuoteClose:
      return end_layout("quote bracket");
      break;
    default:
      break;
  }
  return FAIL;
}

static Symbol process_token_symop(bool whitespace, Lexed next) {
  switch (next) {
    case LDollar:
      return finish_if_valid(SPLICE, "symop");
      break;
    case LDotDot:
      SEQ(finish_if_valid(DOTDOT, "symop"));
      return tight_op(whitespace, QUAL_DOT);
    case LDotId:
      SEQ(finish_if_valid(whitespace ? PREFIX_DOT : TIGHT_DOT, "symop"));
      return tight_op(whitespace, QUAL_DOT);
    case LDotSymop:
      return tight_or_varsym(whitespace, QUAL_DOT);
    case LDotOpen:
      return finish_if_valid(whitespace ? PREFIX_DOT : TIGHT_DOT, "symop");
      break;
    case LBang:
      return infix_or_varsym(whitespace, PREFIX_BANG, TIGHT_BANG);
    case LTilde:
      return infix_or_varsym(whitespace, PREFIX_TILDE, TIGHT_TILDE);
    case LAt:
      return infix_or_varsym(whitespace, PREFIX_AT, TIGHT_AT);
    case LPercent:
      return prefix_or_varsym(whitespace, PREFIX_PERCENT);
    case LSymop:
      if (char0(':')) return finish_symop(CONSYM);
      else return finish_symop(VARSYM);
    default:
      break;
  }
  return FAIL;
}

static Symbol process_token_interior(Lexed next) {
  switch (next) {
    case LBraceClose:
      SEQ(end_layout_brace());
      return token_end_layout_texp();
    // Skip layout start
    case LModule:
      return FAIL;
    case LSemi:
      return explicit_semicolon();
    case LBracketOpen:
      return finish(QQ_START, "qq_start");
    default:
      SEQ(process_token_safe(next));
      return start_layout_interior(next);
  }
  return FAIL;
}

static Symbol process_token_init(uint32_t indent, Lexed next) {
  switch (next) {
    case LModule:
      push_context(ModuleHeader, 0);
      return update_state("init");
    case LBraceOpen:
      advance_over(0);
      MARK("init brace");
      push_context(Braces, indent);
      return finish(START, "init");
    default:
      push_context(DeclLayout, indent);
      return finish(START, "init");
  }
}

// --------------------------------------------------------------------------------------------------------
// Actions that are executed for interior positions
// --------------------------------------------------------------------------------------------------------

static Symbol interior(bool whitespace) {
  Lexed next = lex(false);
  dbg("interior, column %d, ws %d, token %s\n", column(), whitespace, token_names[next]);
  SEQ(resolve_semicolon(next));
  SEQ(process_token_interior(next));
  SEQ(process_token_symop(whitespace, next));
  return FAIL;
}

// --------------------------------------------------------------------------------------------------------
// Newline actions
// --------------------------------------------------------------------------------------------------------

/**
 * Multi-way if layouts are exempt from automatic semicolon generation in GHC.
 */
static Symbol semicolon() {
  if (
      is_semicolon_context()
      &&
      !(newline->no_semi || newline->skip_semi)
      &&
      indent_lesseq(newline->indent)
     ) {
    newline->no_semi = true;
    return finish(SEMICOLON, "newline");
  }
  else return FAIL;
}

/**
 * `NoSpace` + `newline_init()` means that we're at the very beginning of the file, where we start in `NResume` mode
 * without a newline character that can tell us where we are.
 */
static Symbol newline_extras(Space space) {
  bool bol = space == BOL || (space == NoSpace && newline_init());
  Lexed next = lex_extras(bol);
  dbg("newline extras token: %s\n", token_names[next]);
  return process_token_safe(next);
}

// Don't finish newline processing before pragmas – they are indicators of layout indent, but since they are extras,
// they cannot consume a semicolon, so when there's a pragma on a line of its own, we would get two semicolons if we
// finished here.
// It's guaranteed that the newline state was committed at least once because `newline_lookahead` sets `unsafe` when
// finding a pragma.
static Symbol newline_process() {
  dbg("newline post\n");
  uint32_t indent = newline->indent;
  Lexed end = newline->end;
  SEQ(end_layout_indent());
  SEQ(process_token_safe(end));
  Space space = skip_space_and_newline();
  MARK("newline_post");
  if (newline->unsafe) SEQ(newline_extras(space));
  if (!newline->eof) SEQ(start_layout_newline());
  // TODO it is only necessary to run this late because of very few situations, like nondecreasing indent.
  // But it has the consequence that whitespace is included in the parent in nested layouts.
  // Maybe there's a way to run it before and after `start_layout_newline` with conditions.
  SEQ(semicolon());
  reset_newline();
  if (uninitialized()) SEQ(process_token_init(indent, end));
  else SEQ(process_token_symop(true, end));
  return update_state("newline final");
}

static Symbol newline_post() {
  Symbol res = newline_process();
  if (newline_init()) newline->state = NProcess;
  return res;
}

static void newline_lookahead() {
  for (;;) {
    switch (PEEK) {
      NEWLINE_CASES:
        S_SKIP;
        newline->indent = 0;
        break;
      case '\t':
        S_SKIP;
        newline->indent += 8;
        break;
      default:
        if (is_space_char(PEEK)) {
          S_SKIP;
          newline->indent++;
          break;
        }
        reset_lookahead();
        newline->end = lex(newline->indent == 0);
        // Newlines without extras are only safe if `lex` didn't advance the lexer over non-whitespace.
        newline->unsafe |= !no_lookahead();
        switch (newline->end) {
          case LEof:
            newline->indent = 0;
            newline->eof = true;
            return;
          // If/then blocks can have semicolons, but don't have a layout.
          // Allowing layout semicolons costs 100kB.
          case LThen:
          case LElse:
          case LSemi:
            newline->no_semi = true;
            return;
          case LBlockComment:
            newline->indent = consume_block_comment(newline->indent + 2);
            break;
          case LLineComment:
            newline->indent = 0;
            take_line();
            break;
          case LCppElse:
            cpp_else(false);
            take_line();
            break;
          case LCpp:
            take_line();
            break;
          default:
            return;
        }
    }
  }
}

static Symbol newline_start() {
  dbg("newline lookahead\n");
  newline->state = NInit;
  newline_lookahead();
  if (newline->unsafe) return update_state("newline lookahead");
  else return newline_post();
}

static Symbol newline_resume() {
  dbg("newline resume\n");
  uint32_t indent = newline->indent;
  // Skip space between the pragma end and the next token, which might be the first real token (or another pragma or
  // comment, or newline).
  // We don't want to count the space as indent.
  skip_space();
  reset_newline();
  newline->indent = indent;
  newline->state = NResume;
  return newline_start();
}

// --------------------------------------------------------------------------------------------------------
// Initial actions
// --------------------------------------------------------------------------------------------------------

/**
 * These are conditioned only on symbols and don't advance, except for `qq_body`, which cannot fail.
 */
static Symbol pre_ws_commands() {
  SEQ(tuple_context());
  SEQ(start_brace());
  SEQ(end_brace());
  // Leading whitespace must be included in the node.
  if (valid(QQ_BODY)) return qq_body();
  if (newline_active()) SEQ(newline_post());
  else if (newline->state == NResume) SEQ(newline_resume());
  return FAIL;
}

static Symbol scan() {
  MARK("main");
  SEQ(pre_ws_commands());
  bool whitespace = skip_space();
  if (is_newline(PEEK)) return newline_start();
  else if (not_eof()) return interior(whitespace);
  return FAIL;
}

#if DEBUG

static Symbol scan_debug() {
  if (debug_init()) return update_state("debug init parse buffer");
  Symbol result = scan();
  debug_finish(result);
  return result;
}

#endif

static bool process_result(Symbol result) {
  if (result == FAIL && is_eof() && no_lookahead()) {
    MARK("eof whitespace");
    // Inlined `end_layout` because of perf glitch
    if (valid(END)) result = end_layout_unchecked("eof");
    else if (valid(SEMICOLON)) result = finish(SEMICOLON, "eof");
    else {
      result = force_end_context();
      if (result == FAIL) {
        dbg("eof | context cap: %d | lookahead cap: %d | parse cap: %d\n",
          contexts->cap, state->lookahead->cap, state->parse.cap);}
    }
  }
  return set_result_symbol(result);
}


static bool eval() {
  if(after_error()) return false;
#if DEBUG
  Symbol result = scan_debug();
#else
  Symbol result = scan();
#endif
  return process_result(result);
}

// --------------------------------------------------------------------------------------------------------
// API
// --------------------------------------------------------------------------------------------------------

typedef struct {
  bool disabled;
  Newline newline;
  unsigned contexts;
#if DEBUG
  unsigned parse;
#endif
} Persist;

/**
 * This function allocates the persistent state of the parser that is passed into the other API functions.
 */
void *tree_sitter_haskell_external_scanner_create() {
  State *state = calloc(sizeof(State), 1);
  VEC_RESIZE(&state->contexts, 8);
  state->lookahead = calloc(sizeof(Lookahead), 1);
  VEC_RESIZE(state->lookahead, 8);
#if DEBUG
  VEC_RESIZE(&state->parse, 20);
#endif
  return state;
}

/**
 * Main logic entry point.
 * Since the state is a singular vector, it can just be cast and used directly.
 */
bool tree_sitter_haskell_external_scanner_scan(void *payload, TSLexer *lexer, const bool *valid_symbols) {
  Env env_ = env_new(lexer, valid_symbols);
  env = &env_;
  state = (State*) payload;
  contexts = &state->contexts;
  newline = &state->newline;
  return eval();
}

unsigned tree_sitter_haskell_external_scanner_serialize(void *payload, char *buffer) {
  State *state = (State *) payload;
  Persist *persist = (Persist *) buffer;
  *persist = (Persist) {.disabled = state->disabled, .newline = state->newline, .contexts = state->contexts.len};
#if DEBUG
  persist->parse = state->parse.len;
#endif
  unsigned contexts_size = persist->contexts * sizeof(Context);
  unsigned to_copy = contexts_size + sizeof(Persist);
  if (to_copy > TREE_SITTER_SERIALIZATION_BUFFER_SIZE) return 0;
  memcpy(buffer + sizeof(Persist), state->contexts.data, contexts_size);
#if DEBUG
  to_copy = serialize_parse_lines(buffer + sizeof(Persist) + contexts_size, &state->parse, to_copy);
#endif
  return to_copy;
}

void tree_sitter_haskell_external_scanner_deserialize(void *payload, const char *buffer, unsigned length) {
  State *state = (State *) payload;
  Persist p;
  Persist *persist;
  if (length > 0)
    persist = (Persist *) buffer;
  else {
    p = (Persist) {.disabled = false};
    persist = &p;
    persist->newline.state = NResume;
  }
  unsigned contexts_size = persist->contexts * sizeof(Context);
  state->disabled = persist->disabled;
  state->newline = persist->newline;
  VEC_GROW(&state->contexts, persist->contexts);
  state->contexts.len = persist->contexts;
  memcpy(state->contexts.data, buffer + sizeof(Persist), contexts_size);
  state->lookahead->len = 0;
  state->lookahead->offset = 0;
  VEC_GROW(state->lookahead, 8);
#if DEBUG
  deserialize_parse_lines(buffer + sizeof(Persist) + contexts_size, &state->parse, persist->parse);
#endif
}

void tree_sitter_haskell_external_scanner_destroy(void *payload) {
  State *state = (State*) payload;
#if DEBUG
  palette();
  ParseLines *parse = &state->parse;
  for (unsigned i = 0; i < parse->len; i++) VEC_FREE(&parse->data[i]);
  VEC_FREE(parse);
#endif
  VEC_FREE(&state->contexts);
  VEC_FREE(state->lookahead);
  free(state);
}
