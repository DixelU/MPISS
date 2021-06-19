function rand_normal_bm() {
	var u = 0,
		v = 0;
	while (u === 0) u = Math.random(); //Converting [0,1) to (0,1)
	while (v === 0) v = Math.random();
	return Math.sqrt(-2.0 * Math.log(u)) * Math.cos(2.0 * Math.PI * v);
}

function generateTown(n_kids, n_teens, n_mature, n_old, house_count, transport_count, jobs_count, magazine_count, school_count, track_regens_count, daylength) {
	var population_counters = [n_kids, n_teens, n_mature, n_old];
	var ages = ["kid", "teen", "mature", "old"];
	var places = ["H", "T", "W", "S", "M"];
	var objcnt = {
		"H": house_count,
		"T": transport_count,
		"W": jobs_count,
		"M": magazine_count,
		"S": school_count,
	}
	var fixed_shedules = [
		[{"what": "T", "mid": 1, "len_dist": 1, "id_d": 15}, {"what": "S", "mid": 3, "len_dist": 0.5, "id_d": 0}],
		[{"what": "T", "mid": 1, "len_dist": 1, "id_d": 10}, {"what": "S", "mid": 6, "len_dist": 2., "id_d": 0 }, {"what": "M", "mid": 3, "len_dist": 0.5, "id_d": 19}],
		[{"what": "T", "mid": 2, "len_dist": 0.5, "id_d": 5}, {"what": "W", "mid": 9, "len_dist": 3., "id_d": 0}, {"what": "M", "mid": 2, "len_dist": 1., "id_d": 10}],
		[{"what": "H", "mid": 4, "len_dist": 3., "id_d": 0}, {"what": "M", "mid": 3, "len_dist": 0., "id_d": 3}],
		[{"what": "H", "mid": 2, "len_dist": 3., "id_d": 0}, {"what": "M","mid": 5, "len_dist": 0., "id_d": 20}]
	]
	let age_shedule_types = [
		[{"id": 0, "p": 1}],
		[{"id": 0, "p": 0.5}, {"id": 1, "p": 0.95}, {"id": 2, "p": 1}],
		[{"id": 1, "p": 0.05}, {"id": 4, "p": 0.15}, {"id": 2, "p": 0.85}, {"id": 3, "p": 1}],
		[{"id": 3, "p": 1}]
	]
	let random_select_sheule = (age_type_id) => {
		let r = Math.random();
		let n = 0;
		while (age_shedule_types[age_type_id][n].p < r) n++;
		return age_shedule_types[age_type_id][n];
	}
	let town_array = [];
	var convert_time = (time) => (daylength * time / 24) >>> 0;
	for (age_group in population_counters) {
		let N = population_counters[age_group];
		while (N--) {
			let shedule_track = "";
			let lockdown_track = "";
			let lokdown_loop_length = 24 * 5;
			let rel_time = (Math.random() * lokdown_loop_length) >>> 0;
			let closest_magazine = (magazine_count * Math.random()) >>> 0;
			let house = (house_count * Math.random()) >>> 0;
			if (age_group > 1)
				lockdown_track = `H:${house}:${rel_time}; M:${closest_magazine}:1; H:${house}:${lokdown_loop_length-1-rel_time};`;
			let home_time = convert_time(8 + rand_normal_bm() * 1.5);
			let time_left = convert_time(24) - home_time;
			let external_fixed_rnd = Math.random();
			for (let i = 0; i < track_regens_count; i++) {
				shedule_track += `H:${house}:${home_time}; `
				let last_transport = "";
				var type = random_select_sheule(age_group);
				var str = "";
				for (val of fixed_shedules[type.id]) {
					let shedule_length = (val.mid + val.len_dist * rand_normal_bm());
					if (shedule_length <= 0 || !convert_time(shedule_length))
						continue;
					time_left -= convert_time(shedule_length);
					let random_id = Math.random() * objcnt[val.what];
					if (!val.id_d)
						random_id = external_fixed_rnd * objcnt[val.what]
					let buffer_string = `${val.what}:${random_id>>>0}:${convert_time(shedule_length)}`;
					if (val.what == 'T')
						last_transport = buffer_string;
					shedule_track += buffer_string + "; ";
				}
				if (last_transport != "")
					shedule_track += last_transport + `; `;
				shedule_track += `H:${house}:${time_left}; `
			}
			new_ticket = {};
			new_ticket.age = ages[age_group];
			new_ticket.shedule_track = shedule_track;
			new_ticket.special_track = lockdown_track;
			town_array.push(new_ticket);
		}
	}
	return town_array;
}