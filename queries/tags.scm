(
 (haddock)? @doc
 .
 [

  (header (module) @name) @definition.module

  (declarations
   [
    (function name: (variable) @name) @definition.function
    (bind name: (variable) @name) @definition.variable
    ])

  (data_type name: (name) @name) @definition.class

  (newtype name: (name) @name) @definition.class

  (class_declarations
    [
     (function name: (variable) @name)
     (bind name: (variable) @name)
    ] @definition.method)

  (instance_declarations
    [
     (function name: (variable) @name)
     (bind name: (variable) @name)
    ] @definition.method)

  (class name: (name) @name) @definition.interface

  (instance name: (name) @name) @reference.implementation

 ]
 (#strip! @doc "^\s*--+\\s*(\\|\\s*)?")
 (#select-adjacent! @doc @definition.function)
)

(pattern/variable) @definition.variable

(expression/variable) @reference.variable

(expression/apply) @reference.call
