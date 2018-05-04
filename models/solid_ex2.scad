

difference() {
	union() {
		translate(v = [0, 3.4000000000, 0]) {
			cylinder(h = 16, r = 5);
		}
		translate(v = [-15, 0, 0]) {
			rotate(a = [0, 90, 0]) {
				cylinder(h = 30, r = 6);
			}
		}
	}
	union() {
		translate(v = [0, 3.4000000000, -15]) {
			cylinder(h = 45, r = 3);
		}
		translate(v = [-30, 0, 0]) {
			rotate(a = [0, 90, 0]) {
				cylinder(h = 60, r = 3);
			}
		}
	}
}