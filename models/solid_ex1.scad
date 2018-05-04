

difference() {
	union() {
		cube(size = 5);
		translate(v = [5, 0, 0]) {
			sphere(r = 5);
		}
	}
	cylinder(h = 6, r = 2);
}