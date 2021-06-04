create database wdvc;

use wdvc;

create table depart (
    id bigint unsigned auto_increment,
    code varchar(50) not null,
    name varchar(100),
    username varchar(32) not null,
    primary key (id),
    unique key uk01 (code),
    unique key uk02 (username)
) engine=InnoDB default charset=utf8;

create table weight_log (
    id bigint unsigned auto_increment,
    depart_id bigint unsigned not null,
    weight double not null,
    weight_date datetime not null,
    primary key (id),
    foreign key fk01 (depart_id) references depart(id),
    index ix01 (depart_id),
    index ix02 (weight_date)
) engine=InnoDB default charset=utf8;

delimiter $$
create procedure create_depart (
    in p_code varchar(50), in p_name varchar(100), 
    in p_username varchar(32))
    modifies sql data
begin
    declare v_code varchar(50);
    declare v_username varchar(50);

    set v_code = trim(p_code);
    set v_username = trim(p_username);
    if instr(v_username, '@') = 0 then
        set v_username = concat(v_username, '@%');
    end if;

    insert into depart(code, name, username)
    values (v_code, p_name, v_username);
    commit;
end$$
delimiter ;

delimiter $$
create procedure alter_depart_code (
    in p_code varchar(50), in p_new_code varchar(50))
    modifies sql data
begin
    declare v_code varchar(50);
    declare v_new_code varchar(50);
    declare v_msg varchar(1024);

    set v_code = trim(p_code);
    set v_new_code = trim(p_new_code);

    update depart set code = v_new_code
    where code = v_code;
    if row_count() = 0 then
        set v_msg = concat('No such depart with code ', v_code);
    	signal sqlstate '45000' set message_text = v_msg;
    end if;
    commit;
end$$
delimiter ;

delimiter $$
create procedure alter_depart_name (
    in p_code varchar(50), in p_new_name varchar(100))
    modifies sql data
begin
    declare v_code varchar(50);
    declare v_msg varchar(1024);

    set v_code = trim(p_code);

    update depart set name = p_new_name
    where code = v_code;
    if row_count() = 0 then
        set v_msg = concat('No such depart with code ', v_code);
    	signal sqlstate '45000' set message_text = v_msg;
    end if;
    commit;
end$$
delimiter ;

delimiter $$
create procedure alter_depart_username (
    in p_code varchar(50), in p_new_username varchar(32))
    modifies sql data
begin
    declare v_code varchar(50);
    declare v_new_username varchar(50);
    declare v_msg varchar(1024);

    set v_code = trim(p_code);
    set v_new_username = trim(p_new_username);
    if instr(v_new_username, '@') = 0 then
        set v_new_username = concat(v_new_username, '@%');
    end if;

    update depart set username = v_new_username
    where code = v_code;
    if row_count() = 0 then
        set v_msg = concat('No such depart with code ', v_code);
    	signal sqlstate '45000' set message_text = v_msg;
    end if;
    commit;
end$$
delimiter ;

delimiter $$
create procedure drop_depart (
    in p_code varchar(50))
    modifies sql data
begin
    declare v_code varchar(50);

    set v_code = trim(p_code);

    delete from depart where code = v_code;
    commit;
end$$
delimiter ;

delimiter $$
create procedure log_weight (
    in p_weight double, in p_username varchar(32))
    modifies sql data
begin
    declare v_msg varchar(1024);
    declare v_not_found bool;
    declare v_depart_id bigint;

    declare continue handler for not found 
    set v_not_found = true;

    if user() not like p_username then
    	set v_msg = concat('You can not write log as user ', 
                           p_username);
    	signal sqlstate '45000' set message_text = v_msg;
    end if;

    set v_not_found = false;
    select id into v_depart_id from depart 
    where username = p_username;
    if v_not_found then
    	set v_msg = concat('User does not registered: ', 
                           p_username);
    	signal sqlstate '45000' set message_text = v_msg;
    end if;
    
    insert into weight_log(depart_id, weight, weight_date)
    values (v_depart_id, p_weight, sysdate());
    commit;
end$$
delimiter ;
