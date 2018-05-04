use <MCAD/2Dshapes.scad>
use <MCAD/boxes.scad>

//  # 角の丸い四角形
linear_extrude(height=10)
    roundedSquare(pos=[30,30],r=5);
// # 同様の図形をroundedBoxで表示、原点を中心に表示しているので、Z軸がずれる
translate([70, 0, 0])
    roundedBox([30, 30, 10], 5, true);
    
// # 楕円    
translate([0, 70, 0])
    linear_extrude(height=10)
        ellipse(50,75);