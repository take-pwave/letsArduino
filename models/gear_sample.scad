use <MCAD/involute_gears.scad>

gear (number_of_teeth=6,  // 歯車の数
    circular_pitch=300,
    hub_diameter=0,
    rim_width=5,
    rim_thickness=5,              // 外側の
    ressure_angle=31);
rotate ([0,0,360*5/6])
translate ([22.5,0,0])
gear (number_of_teeth=21,// 歯車の数
    circular_pitch=300,
    bore_diameter=5,            // 中心の穴の直径
    hub_diameter=10,           // 中心部の直径
    hub_thickness=5,             // 中心部の厚さ
    rim_width=1,                     // 外側の厚さ
    rim_thickness=5,              // 外側の厚さ
    gear_thickness=3,            //ギアの厚さ
    pressure_angle=31);