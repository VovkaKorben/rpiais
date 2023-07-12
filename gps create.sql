DROP TABLE if exists `gps_track`;
DROP TABLE if exists `gps_total`;
DROP TRIGGER if exists `pos_added`;
DROP FUNCTION IF EXISTS `calc_dist`;
DROP FUNCTION IF EXISTS `vincenty`;

CREATE TABLE `gps_track` (
  `tm` bigint(20) NOT NULL ,
  `sessid` int(11) NOT NULL,
  `lon` FLOAT(20,6) NOT NULL,
  `lat` FLOAT(20,6) NOT NULL,
KEY `idx_tm` (`tm`) USING BTREE,  
KEY `idx_sess` (`sessid`) USING BTREE
-- ,  KEY `idx_pos` (`lon`,`lat`) USING BTREE
) ENGINE=InnoDB DEFAULT CHARSET=ascii COLLATE=ascii_general_ci ROW_FORMAT=COMPRESSED;
CREATE TABLE `gps_total` (
  `sessid` int(11),
  `points` int(11), 
  `dist` double,
  UNIQUE KEY `idx_sess` (`sessid`) USING BTREE  
) ENGINE=InnoDB DEFAULT CHARSET=ascii COLLATE=ascii_general_ci ROW_FORMAT=COMPRESSED;

DELIMITER $$
CREATE TRIGGER gps_update BEFORE INSERT ON gps_track
FOR EACH ROW
BEGIN
	DECLARE mysessid int;
	DECLARE mypoints int;
	DECLARE mydist double;
	DECLARE x0 FLOAT;
	DECLARE y0 FLOAT;
	DECLARE upd_dist double;

	SELECT sessid, points, dist INTO mysessid, mypoints, mydist
	FROM gps_total WHERE gps_total.sessid = NEW.sessid;

	if mysessid is NULL THEN -- create new session row
		insert into gps_total (`sessid`, `points`, `dist`) values (NEW.sessid,1,0.0);
	ELSE -- row already exists, calculate updated distance
		SELECT lon,lat INTO x0,y0
		FROM gps_track
		WHERE gps_track.sessid = NEW.sessid
		ORDER BY tm DESC
		LIMIT 1;
		
		SET upd_dist = vincenty(x0,y0,NEW.lon,NEW.lat);

		UPDATE gps_total
		SET points = mypoints+1,
		dist = mydist + upd_dist
		WHERE sessid = NEW.sessid;

	END IF;

END; 

CREATE FUNCTION `vincenty`( x0 FLOAT, y0 FLOAT, x1 FLOAT, y1 FLOAT) RETURNS double
BEGIN

DECLARE req DOUBLE DEFAULT 6378137.0 ;
DECLARE flat DOUBLE DEFAULT 1 / 298.257223563 ;
DECLARE rpol DOUBLE DEFAULT (1 - flat) * req ;
DECLARE sin_sigma DOUBLE ;
DECLARE cos_sigma DOUBLE ;
DECLARE sigma DOUBLE ;
DECLARE sin_alpha DOUBLE ;
DECLARE cos_sq_alpha DOUBLE ;
DECLARE cos2sigma DOUBLE ;

DECLARE C DOUBLE ;
DECLARE lam_pre DOUBLE ;
DECLARE u1 DOUBLE ;
DECLARE u2 DOUBLE ;
DECLARE lon DOUBLE ;
DECLARE lam DOUBLE ;
DECLARE tol DOUBLE ;
DECLARE diff DOUBLE ;
DECLARE distance double ;
DECLARE usq DOUBLE ;
DECLARE A DOUBLE ;
DECLARE B DOUBLE ;
DECLARE delta_sig DOUBLE ;

SET y0 = PI() * y0 / 180.0 ;
SET y1 = PI() * y1 / 180.0 ;
SET x0 = PI() * x0 / 180.0 ;
SET x1 = PI() * x1 / 180.0 ;
SET u1 = atan((1 - flat) * tan(y1)) ;
SET u2 = atan((1 - flat) * tan(y0)) ;
SET lon = x0 - x1 ;
SET lam = lon ;



SET tol = power(10.0, - 12.0) ;
SET diff = 1.0 ;
WHILE (abs(diff) > tol) DO
	SET sin_sigma = SQRT(	POWER( COS(u2) * SIN(lam), 2) + 	POWER( COS(u1) * SIN(u2) - SIN(u1) * COS(u2) * COS(lam), 2)) ;
	SET cos_sigma = sin(u1) * sin(u2) + cos(u1) * cos(u2) * cos(lam) ;
	SET sigma = atan2(sin_sigma , cos_sigma) ;
	set sin_alpha = (cos(u1) * cos(u2) * sin(lam)) / sin_sigma;
	set cos_sq_alpha = 1 - pow(sin_alpha, 2.0);
	set cos2sigma = cos_sigma - ((2 * sin(u1) * sin(u2)) / cos_sq_alpha);
	set C = (flat / 16) * cos_sq_alpha * (4 + flat * (4 - 3 * cos_sq_alpha));
	set lam_pre = lam;
	set lam = lon + (1 - C) * flat * sin_alpha * (sigma + C * sin_sigma * (cos2sigma + C * cos_sigma * (2 * power(cos2sigma, 2.0) - 1)));
	set diff = abs(lam_pre - lam);
END WHILE;


set distance = rpol * (	sigma - flat * sin_alpha * (		sigma + C * sin_sigma * (			cos2sigma + C * cos_sigma * (				2 * power(cos2sigma, 2.0) - 1))));
RETURN distance ;

END $$
DELIMITER ;

insert into `gps_track` (`tm`,`sessid`,`lon`,`lat`) VALUES (100,0,10.0,20.0);
insert into `gps_track` (`tm`,`sessid`,`lon`,`lat`) VALUES (200,0,30.0,40.0);
-- insert into `gps_track` (`tm`,`sessid`,`lon`,`lat`) VALUES (300,0,12.0,22.0);
-- insert into `gps_track` (`tm`,`sessid`,`lon`,`lat`) VALUES (400,0,13.0,23.0);
-- insert into `gps_track` (`tm`,`sessid`,`lon`,`lat`) VALUES (500,1,10.0,20.0);
-- insert into `gps_track` (`tm`,`sessid`,`lon`,`lat`) VALUES (600,1,11.0,21.0);
-- insert into `gps_track` (`tm`,`sessid`,`lon`,`lat`) VALUES (700,1,12.0,22.0);
-- 