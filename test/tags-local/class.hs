a = 1

-- | Documentation
class A a where
  meth :: Int -> a
  meth a = a

-- | Documentation
instance A Int where
  meth a = a
