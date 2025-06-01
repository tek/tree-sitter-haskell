module A.B where

-- | Documentation
class A a where
  -- | Documentaton
  meth1 :: Int -> a
  meth1 a = a

  -- | Documentaton
  meth2 :: Int -> a
  meth2 a = a

-- | Documentation
instance A Int where
  -- | Documentaton
  meth1 a = a

  -- | Documentaton
  meth2 a = a
