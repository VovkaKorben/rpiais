-- clear previous IDs
DELETE FROM `mapselect`;

-- add suitable shape IDs
INSERT INTO `mapselect` 
	SELECT recid 
	FROM shapes 
	WHERE %.10f < maxx AND %.10f < maxy AND %.10f > minx AND %.10f > miny
	%s;

-- get shape parameters
SELECT
	shapes.recid,
	shapes.parts,
	shapes.points,
	shapes.minx,
	shapes.miny,
	shapes.maxx,
	shapes.maxy
FROM `mapselect` 
LEFT JOIN shapes ON mapselect.recid = shapes.recid;

-- get count of points in groups
select points.recid, count( DISTINCT points.partid) cnt
from mapselect 
LEFT JOIN points ON mapselect.recid = points.recid
GROUP BY points.recid;

-- retrieve all points
SELECT	points.*
FROM	`mapselect`
LEFT JOIN points ON mapselect.recid = points.recid
ORDER BY	points.recid ASC,	points.partid ASC,	points.pointid ASC;
-- -- -- -- --