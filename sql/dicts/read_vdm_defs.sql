SELECT
vdm_field_list.msg_id,
vdm_field_list.`start`,
vdm_field_list.len,
vdm_field_list.ref,
vdm_field_descr.type,
vdm_field_descr.def,
vdm_field_descr.exp
from `vdm_field_list`,`vdm_field_descr`
-- where `vdm_field_list`.`ref` is not null and
where 
`vdm_field_list`.`ref` = `vdm_field_descr`.`name`
order by 
`vdm_field_list`.`msg_id` asc,
`vdm_field_list`.`start` asc
;