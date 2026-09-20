const {
  parens,
  brackets,
  unboxed,
  string_char,
  multiline_string_char,
  decimal,
  binary_literal,
  octal_literal,
  hex_literal,
} = require('./util.js')

const exponent = /[eE][+-]?[0-9_]+/
const magic_hash = rule => token(seq(rule, optional(token.immediate(/##?/))))
const magic_hash_typed = rule => token(seq(rule, optional(token.immediate(/#(#|(Word|Int)(8|16|32|64)?)?/))))

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
        repeat(string_char),
        '"',
      ),
      seq(
        '"""',
        repeat(multiline_string_char),
        '"""'
      ),
    ),
  ),

  _integer_literal: _ => magic_hash_typed(decimal),
  _binary_literal: _ => magic_hash_typed(binary_literal),
  _octal_literal: _ => magic_hash_typed(octal_literal),
  _hex_literal: _ => magic_hash_typed(hex_literal),

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
