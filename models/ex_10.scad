difference() {
    union() {
        translate([0,3.4,0])
          cylinder(r=5.0, h=16.0);
        translate([-15.0,0,0])
          rotate([0,90,0])
            cylinder(r=6.0, h=30.0);        
    }
    
    translate([0, 3.4, -15.0])
        cylinder(r=3.0, h=3*15.0);
    translate([-30.0, 0,0])
        rotate([0,90,0])
            cylinder(r=3.0, h=2*30.0);
}