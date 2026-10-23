import probe5 as p
g = p.make_generic()
print("shared_ptr<GenericObject> (non-virtual root) ->", type(g).__name__, "number_band =", g.number_band(), "psize =", g.psize(), "add =", g.add())
