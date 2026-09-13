{- |
 NOTE: Because Haskell's `-- ^` comments are parsed as (haddock) and not (comment),
 we cannot use `-- ^` assertions.
-}

{-# LANGUAGE QuasiQuotes #-}
-- <- keyword.directive

{-| Main module -}
-- <- comment.documentation
module
 -- <- keyword.import
  Main
    -- <- module
  ( main
    -- <- variable
  ) where
    -- <- keyword

import Prelude hiding (show)
-- <- keyword.import
import Prelude hiding (show)
         -- <- module
import Prelude hiding (show)
                -- <- keyword
import Prelude hiding (show)
                        -- <- variable
import Data.Map (fromList)
        -- <- module
import qualified Data.Map as Map
-- <- keyword.import
import qualified Data.Map as Map
                  -- <- module
import qualified Data.Map as Map
                       -- <- module
import qualified Data.Map as Map
                             -- <- module
import qualified Chronos
                  -- <- module
import qualified Chronos as C
-- <- keyword.import
import qualified Chronos as C
                   -- <- module
import qualified Chronos as C
                            -- <- module
import FooMod (BarTy (barField))
                      -- <- variable.member

x = mempty { field = 5 }
              -- <- variable.member

data ADT
-- <- keyword
  = A Int
    -- <- constructor
  = B Int
       -- <- type
  | C
    -- <- constructor
  deriving (Eq, Show)
   -- <- keyword
  deriving (Eq, Show)
            -- <- type
  deriving (Eq, Show)
                 -- <- type
mkA x = A x
    -- <- variable.parameter
mkAQualified x = SomeModule.A x
             -- <- variable.parameter
mkAQualified x = SomeModule.A x
                              -- <- variable.parameter

class Ord a => PartialOrd a
-- <- keyword
class Ord a => PartialOrd a
       -- <- type
class Ord a => PartialOrd a
          -- <- variable
class Ord a => PartialOrd a
                -- <- type
class Ord a => PartialOrd a
                          -- <- variable
instance Ord ADT where
-- <- keyword
instance Ord ADT where
          -- <- type
instance Ord ADT where
              -- <- type

newtype Rec
-- <- keyword
newtype Rec
        -- <- type
  = Rec
     -- <- constructor
    { fieldA :: Double
    --  <- punctuation.bracket
    , fieldB :: Double
       -- <- variable.member
    , fieldC :: Double
                 -- <- type
    }
    -- <- punctuation.bracket
    deriving Eq
             -- <- type
recordWildCard Rec { field } = field
                      -- <- variable.member
recordDotSyntax rec = rec.field
                            -- <- variable.member


main :: IO ()
-- <- function
main :: IO ()
     -- <- operator
main :: IO ()
        -- <- type
main :: IO ()
           -- <- type
main = undefined
-- <- function
main = undefined
        -- <- keyword.exception

someFunc0 :: Int -> Int
                  -- <- operator
someFunc0 x = someFunc1 x
          -- <- variable.parameter
someFunc0 x = someFunc1 x
              -- <- function.call
  where
   -- <- keyword
    someFunc1 _ = 5
     -- <- function
    someFunc2 _ = 5
                  -- <- number
scopedTypeParam (x :: Int) = someFunc x
                  -- <- variable.parameter
scopedTypeParam (x :: Int) = someFunc x
                      -- <- type
scopedTypeParam (Just x :: Int) = someFunc x
                  -- <- constructor
scopedTypeParam (Just x :: Int) = someFunc x
                     -- <- variable.parameter
scopedTypeParam (Just x :: Int) = someFunc x
                            -- <- type
scopedTypeParam (f :: Int -> Int) = someFunc x
                 -- <- function

someInfix :: Integral a => a -> Double
              -- <- type
someInfix :: Integral a => a -> Double
                      -- <- variable
someInfix :: Integral a => a -> Double
                         -- <- operator
someInfix :: Integral a => a -> Double
                           -- <- variable
someInfix :: Integral a => a -> Double
                                -- <- type
someInfix x = fromIntegral x `myAdd` floatVal
                -- <- function.call
someInfix x = fromIntegral x `myAdd` floatVal
                           -- <- variable.parameter
someInfix x = fromIntegral x `myAdd` floatVal
                                -- <- operator
someInfix x = fromIntegral x `myAdd` floatVal
                                       -- <-  variable
  where
    myAdd :: Num a => a -> a
    -- <- function
    myAdd x y = x + y
          -- <- variable.parameter
    myAdd x y = x + y
                -- <- variable.parameter
    myAdd x y = x + y
                    -- <- variable.parameter
    floatVal :: Double
    -- <- variable
    floatVal = 5.5
    -- <- variable
    floatVal = 5.5
               -- <- number.float
    intVal :: Int
    -- <- variable
    intVal = getInt 5
    -- <- variable
    boolVal :: Bool
    -- <- variable
    boolVal = bool False True $ 1 + 2 == 3
    -- <- variable
    refVal = boolVal
    -- <- variable
    namespacedRecord = NS.Rec { field = bar }
    -- <- variable
    record = Rec { field = bar }
    -- <- variable
    constructorRef = A
    -- <- function
    isInt :: Either Double Int -> Bool
    -- <- function
    isInt eith@Left{} = False
           -- <- variable.parameter
    isInt eith@(Left x) = False
    -- <- function
    isInt eith@(Left x) = False
                    -- <- variable.parameter
    isInt (Left x) = False
               -- <- variable.parameter
    isInt (Right _) = True
    -- <- function

someIOaction :: IO ()
-- <- function
anotherIOaction :: IO ()
anotherIOaction = do
-- <- function
  pure ()

anotherIOaction = do
                   -- <- keyword
  foo <- SomeModule.someFun <$> getArgs
  -- <- variable
  foo <- SomeModule.someFun <$> getArgs
             -- <- module
  foo <- SomeModule.someFun <$> getArgs
                     -- <- function.call
  _ <- someFunc0 =<< someIOAction
        -- <- function.call
  let bar = SomeModule.doSomething $ "a" "b"
      -- <- variable
  let bar = SomeModule.doSomething $ "a" "b"
             -- <- module
  let bar = SomeModule.doSomething $ "a" "b"
                         -- <- function.call
      func x y = x + y - 7
       -- <- function
      func x y = x + y - 7
           -- <- variable.parameter
      func x y = x + y - 7
                 -- <- variable.parameter
      func x y = x + y - 7
                     -- <- variable.parameter
      valueFromList = HashSet.fromList []
       -- <- variable
  when foo $ putStrLn $ T.showt =<< bar
  -- <- function.call
  when foo $ putStrLn $ T.showt =<< bar
        -- <- variable
  when foo $ putStrLn $ T.showt =<< bar
               -- <- function.call
  when foo $ putStrLn $ T.showt =<< bar
                           -- <- function.call

  pure $ func 1 2
  -- <-  function.call
  pure $ func 1 2
         -- <- function.call

intFun :: Int -> Int
intFun = 5
-- <- function

undefinedFun :: Int -> Int
undefinedFun = undefined
-- <- function

getLambda x = \y -> x `SomeModule.someInfix` y
               -- <- variable.parameter
getLambda x = \y -> x `SomeModule.someInfix` y
                        -- < module
getLambda x = \y -> x `SomeModule.someInfix` y
                                   -- <- operator
lambdaTyped = \(y :: Int) -> x
                 -- <- variable.parameter
lambdaPattern = \(Just x) -> x
                      -- <- variable.parameter
lambdaPatternTyped = \(Just x :: Int) -> x
                           -- <- variable.parameter

isVowel = (`elem` "AEIOU")
             -- <- operator
isVowelQualified = (`SomeModule.elem` "AEIOU")
                      -- <- module
isVowelQualified = (`SomeModule.elem` "AEIOU")
                                 -- <- operator

hasVowels = ("AEIOU" `elem`)
                       -- <- operator
hasVowelsQualified = ("AEIOU" `SomeModule.elem`)
                                -- -< module
hasVowelsQualified = ("AEIOU" `SomeModule.elem`)
                                          -- <- operator

quasiQuotedString = [qq|Some string|]
-- <- variable
quasiQuotedString = [qq|Some string|]
                      -- <- function.call
quasiQuotedString = [qq|Some string|]
                         -- <- string
quasiQuotedString2 = [SomeModule.qq|Some string|]
                        -- <- module
quasiQuotedString2 = [SomeModule.qq|Some string|]
                                 -- <- function.call

qualifiedComposition = SomeModule.f . SomeModule.g
                                  -- <- function
qualifiedComposition = SomeModule.f . SomeModule.g
                                                 -- <- variable
takeMVarOrThrow = evaluate <=< takeMVar
                   -- <- function
takeMVarOrThrow = evaluate <=< takeMVar
                                -- <- function
modifyMVarOrThrow v f = modifyMVar v $ f >=> evaluate
                  -- <- variable.parameter
modifyMVarOrThrow v f = modifyMVar v $ f >=> evaluate
                                              -- <- function
assertNonEmpty xs = xs `shouldSatisfy` not . null
                -- <- variable.parameter
assertNonEmpty xs = xs `shouldSatisfy` not . null
                                        -- <- function
assertNonEmpty xs = xs `shouldSatisfy` not . null
                                               -- <- function
(Qu a) |/| (SomeModule.Qu b) = a / b
   -- <- variable.parameter
(Qu a) |/| (SomeModule.Qu b) = a / b
                         -- <- variable.parameter
(Qu a :: Int) |/| (SomeModule.Qu b :: Int) = a / b
   -- <- variable.parameter
(Qu a :: Int) |/| (SomeModule.Qu b :: Int) = a / b
                                -- <- variable.parameter
viewPattern (func -> var) = 5
              -- <- function.call
viewPattern (func -> var) = 5
                      -- <- variable
g (func :: a -> b) x = func y
    -- <- function
lambdaAlias :: LambdaAlias
lambdaAlias _ _ _ = undefined
  -- <- function

composed = f . g
-- ^ function
