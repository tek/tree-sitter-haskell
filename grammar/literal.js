const {
  parens,
  brackets,
  unboxed,
} = require('./util.js')

const decimal = /[0-9][0-9_]*/
const exponent = /[eE][+-]?[0-9_]+/
const hex_exponent = /[pP][+-]?[0-9a-fA-F_]+/
const magic_hash = rule => token(seq(rule, optional(token.immediate(/##?/))))

// A backslash followed by a ^ is a special sort of control character that is always followed by another character.
// Otherwise, any character is permitted after a backslash.
const escaped = /\\(\^)?./

// Both single- and multiline strings allow splitting across line breaks with leading whitespace removed by inserting
// two backslashes – as the last character of the first line and the first non-whitespace character on the next line:
// s = "one \
//        \line"
// This will result in `"one line"`.
const string_gap = /\\\n\s*\\/

module.exports = {

  // ------------------------------------------------------------------------
  // literals
  // ------------------------------------------------------------------------

  // the `choice` here is necessary to avoid integers being parsed as floats
  float: _ => magic_hash(
    seq(
      decimal,
      choice(
        seq(/\.[0-9_]+/, optional(exponent)),
        exponent,
      ),
    ),
  ),

  char: _ => magic_hash(
    choice(
      /'[^']'/,
      /'\\[^ ]*'/,
    ),
  ),

  string: _ => magic_hash(
    choice(
      seq(
        '"',
        repeat(choice(
          /[^\\"\n]/, // Any character that's neither backslash, double quote nor newline needs no special consideration.
          escaped,
          string_gap,
        )),
        '"',
      ),
      seq(
        '"""',
        repeat(choice(
          /[^\\"]/, // Any character that's neither backslash nor double quote needs no special consideration.
          escaped,
          string_gap,
          // In multiline strings, up to two consecutive double quotes are permitted without escaping.
          /"[^"]/,
          /""[^"]/,
        )),
        '"""'
      ),
    ),
  ),

  _integer_literal: _ => magic_hash(decimal),
  _binary_literal: _ => magic_hash(/0[bB][01_]+/),
  _octal_literal: _ => magic_hash(/0[oO][0-7]+/),

  _hex_literal: _ => magic_hash(
    seq(
      /0[xX][0-9a-fA-F_]+/,
      optional(/\.[0-9a-fA-F_]+/),
      optional(hex_exponent),
    )
  ),

  integer: $ => choice(
    $._binary_literal,
    $._integer_literal,
    $._octal_literal,
    $._hex_literal,
  ),

  _stringly: $ => choice(
    $.string,
    $.char,
  ),

  _number: $ => choice(
    $.integer,
    $.float,
  ),

  _plist: $ => brackets($),

  unit: $ => parens($),
  unboxed_unit: $ => unboxed($),

  prefix_tuple: $ => parens($, repeat1(',')),
  prefix_unboxed_tuple: $ => unboxed($, repeat1(',')),
  prefix_unboxed_sum: $ => unboxed($, repeat1($._unboxed_bar)),

  literal: $ => choice(
    $._stringly,
    $._number,
  ),

  _unit_cons: $ => choice(
    $.unit,
    $.unboxed_unit,
  ),

  _tuple_cons: $ => choice(
    $.prefix_tuple,
    $.prefix_unboxed_tuple,
    $.prefix_unboxed_sum,
  ),

}
