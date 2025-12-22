const haskell = require("../grammar.js")
const {
  sep,
} = require('../grammar/util.js')

module.exports = grammar(haskell, {
  name: 'hsc',

  rules: {

    _exp: ($, previous) => choice(previous, $.hsc),

    hsc: $ => seq(
      "#{",
      $.hsc_kind,
      sep(',', $.hsc_arg),
      "}",
    ),

    hsc_kind: $ => $.variable,

    hsc_arg: $ => $.variable,

  },

})
