const haskell = require("../grammar.js")
const {
  sep1,
} = require('../grammar/util.js')

const mk_hsc_directive = ($, name, ...rule) => choice(
  seq(
    '#{',
    field('kind', name),
    ...rule,
    '}'
  ),
  // Same directives with # embedded into directive name to avoid conflicts.
  seq(
    field('kind', alias('#' + name, name)),
    ...rule,
  ),
)

const mk_hsc_directive_type = ($, name) => mk_hsc_directive(
  $,
  name,
  field('type', $.hsc_c_type),
)

const mk_hsc_directive_type_field = ($, name) => mk_hsc_directive(
  $,
  name,
  field('type', $.hsc_c_type),
  ',',
  field('path', $.hsc_c_field)
)

const mk_hsc_directive_expression = ($, name) => mk_hsc_directive(
  $,
  name,
  field('expression', $.hsc_c_expr)
)

module.exports = grammar(haskell, {
  name: 'hsc',

  rules: {

    // Collect all possible hsc directives under this node but don’t use it since
    // each directive is valid in only some contexts and not others,
    // e.g. #peek is never a valid pattern - it will never typecheck because
    // it produces monadic function.
    hsc: $ => choice(
      $._hsc_int_directive,
      $._hsc_str_directive,
      $._hsc_type_directive,
      $._hsc_func_directive,
    ),

    // Hook up hsc directives into Haskell grammar depending
    // on their return type.
    _stringly: ($, previous) => choice(
      previous,
      alias($._hsc_str_directive, $.hsc),
    ),

    _number: ($, previous) => choice(
      previous,
      alias($._hsc_int_directive, $.hsc),
    ),

    expression: ($, previous) => choice(
      previous,
      alias($._hsc_func_directive, $.hsc),
    ),

    type: ($, previous) => choice(
      previous,
      alias($._hsc_type_directive, $.hsc),
    ),

    // Directives returning integer.
    _hsc_int_directive: $ => choice(
      mk_hsc_directive_type($, 'size'),
      mk_hsc_directive_type($, 'alignment'),
      mk_hsc_directive_type_field($, 'offset'),
      mk_hsc_directive_expression($, 'const'),
    ),

    // Directives returning string.
    _hsc_str_directive: $ => mk_hsc_directive_expression($, 'const_str'),

    // Directives returning type name.
    _hsc_type_directive: $ => mk_hsc_directive_type($, 'type'),

    // Directives returning function.
    _hsc_func_directive: $ => choice(
      mk_hsc_directive_type_field($, 'peek'),
      mk_hsc_directive_type_field($, 'poke'),
      mk_hsc_directive_type_field($, 'ptr'),
    ),

    // Incomplete: can have inline C type in here.
    hsc_c_type: $ => seq(
      optional($._hsc_c_restricted_type_specifier),
      $.hsc_c_identifier
    ),
    hsc_c_field: $ => sep1(
      token.immediate('.'),
      $.hsc_c_identifier,
    ),

    // Simplest C expressions that we support for now - just reference to a name,
    // already accounts for 99% of uses.
    hsc_c_expr: $ => $.hsc_c_identifier,

    hsc_c_identifier: $ =>
      token(/(\p{XID_Start}|[a-zA-Z_$])(\p{XID_Continue}|[0-9a-zA-Z_$])*/),

    _hsc_c_restricted_type_specifier: $ => choice('struct', 'union'),

  },

})
