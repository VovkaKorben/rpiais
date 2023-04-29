SELECT	shapes.recid,	shapes.parts,	shapes.points,	shapes.minx,	shapes.miny,	shapes.maxx,	shapes.maxy FROM `mapselect` LEFT JOIN shapes ON mapselect.recid = shapes.recid;
