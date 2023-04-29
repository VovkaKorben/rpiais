select points.recid, count( DISTINCT points.partid) cnt
from mapselect 
LEFT JOIN points ON mapselect.recid = points.recid
GROUP BY points.recid;