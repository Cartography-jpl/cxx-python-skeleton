import gc, obs_nb as p
cam = p.Camera()
igc = p.Igc(cam)                              # Python-constructed, registers in its C++ constructor
print("1. Python-constructed observer notified:", cam.notify(), "count =", igc.count)
print("2. shared_from_this owner is real after passing to C++:", p.owner_is_real(igc))
class PyObs(p.CameraObserver):
    def __init__(self): super().__init__(); self.n = 0
    def notify_update(self, c): self.n += 1
o = PyObs(); cam.add_observer(o)
print("3. Python-subclass observer: notified", cam.notify(), "| o.n =", o.n)
del o; gc.collect()
print("4. after Python observer dropped (only weak ref in C++):", cam.notify(), "(expect 1: just igc)")
k = PyObs(); cam.add_observer_and_keep_reference(k); kid = id(k); del k; gc.collect()
print("5. add_observer_and_keep_reference keeps Python observer alive:", cam.notify(), "(expect 2)")
del igc; gc.collect()
print("6. after igc dropped:", cam.notify(), "(expect 1)")
