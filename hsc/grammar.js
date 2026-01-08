const haskell = require("../grammar.js")
const {
  id_char,
  varid_start_char,
} = require('../grammar/util.js')

const duplicate_magic_hash_for_hsc = rule =>
  token(seq(rule, optional(token.immediate(/##?/))))

module.exports = grammar(haskell, {
  name: 'hsc',

  rules: {

    // Need to fix unboxed tuples because in hsc2hs hashes must
    // be duplicated and unboxed tuples look like (## ... ##)
    _unboxed_open: $ =>
      alias(seq($._paren_open, token.immediate('##')), '(#'),
    _unboxed_close: $ =>
      seq(alias('##)', '#)'), $._cmd_texp_end),

    // Fix labels like foo#
    label: _ => token(seq('##', varid_start_char, id_char)),

    // Fix hash operators
    _operator_hash_head: _ => seq(
      choice('##', token.immediate('##')),
      optional(choice(token.immediate('##'), token.immediate('|'))),
    ),

    // Fix magic hash literals as they need to have their # duplicated.
    float: (_, previous) => duplicate_magic_hash_for_hsc(previous),
    char: (_, previous) => duplicate_magic_hash_for_hsc(previous),
    string: (_, previous) => duplicate_magic_hash_for_hsc(previous),
    _integer_literal: (_, previous) => duplicate_magic_hash_for_hsc(previous),
    _binary_literal: (_, previous) => duplicate_magic_hash_for_hsc(previous),
    _octal_literal: (_, previous) => duplicate_magic_hash_for_hsc(previous),
    _hex_literal: (_, previous) => duplicate_magic_hash_for_hsc(previous),

    // Hook hsc directives into regular Haskell constructs
    calling_convention: ($, previous) => choice(previous, $.hsc),
    safety: ($, previous) => choice(previous, $.hsc),
    _ie_entity: ($, previous) => choice(previous, $.hsc),

    _var: ($, previous) => choice(previous, $.hsc),
    _tycon: ($, previous) => choice(previous, $.hsc),
    _decl_con: ($, previous) => choice(previous, $.hsc),
    _decl_constructor: ($, previous) => choice(previous, $.hsc),

    // Hsc directive definitions
    hsc: $ => choice(
      seq(
        $._cond_hsc_hash,
        '#{',
        $._cmd_hsc_brace_open,
        field('name', $.hsc_directive_name),
        field('body', alias($.hsc_args_nested, $.hsc_args)),
        '}',
        $._cmd_brace_close,
      ),
      seq(
        $._cond_hsc_hash,
        '#',
        field('name', $.hsc_directive_name),
        field('body', alias($.hsc_args_newline, $.hsc_args)),
      ),
    ),

    hsc_directive_name: $ =>
      token(/[a-zA-Z_]+/),
  },

})
