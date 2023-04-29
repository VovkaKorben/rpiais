DELETE FROM `mapselect`;
INSERT INTO `mapselect` SELECT recid FROM shapes WHERE %.10f < maxx AND %.10f < maxy AND %.10f > minx AND %.10f > miny%s;
