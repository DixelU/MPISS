function randn_bm() {
    var u = 0, v = 0;
    while(u === 0) u = Math.random(); //Converting [0,1) to (0,1)
    while(v === 0) v = Math.random();
    return Math.sqrt( -2.0 * Math.log( u ) ) * Math.cos( 2.0 * Math.PI * v );
}

function generateTown(nk,nt,nm,no,hc,tc,wc,mc,sc,trackregenscnt,daylength){
	var population_cnt=[nk,nt,nm,no];
	var ages = ["kid","teen","mature","old"];
	var places = ["H","T","W","S","M"];
	var objcnt = {"H":hc,"T":tc,"W":wc,"M":mc,"S":sc,}
	var fixed_shedules = [
		[
			{"what":"T","mid":1,"len_dist":1,"id_d":15},
			{"what":"S","mid":3,"len_dist":0.5,"id_d":0}
		],
		[
			{"what":"T","mid":1,"len_dist":1,"id_d":10},
			{"what":"S","mid":6,"len_dist":2.,"id_d":0},
			{"what":"M","mid":3,"len_dist":0.5,"id_d":19}
		],
		[
			{"what":"T","mid":2,"len_dist":0.5,"id_d":5},
			{"what":"W","mid":9,"len_dist":3.,"id_d":0},
			{"what":"M","mid":2,"len_dist":1.,"id_d":10}
		],
		[
			{"what":"H","mid":4,"len_dist":3.,"id_d":0},
			{"what":"M","mid":3,"len_dist":0.,"id_d":3}
		],
		[
			{"what":"H","mid":2,"len_dist":3.,"id_d":0},
			{"what":"M","mid":5,"len_dist":0.,"id_d":20}
		]
	]
	let age_shedule_types=[
		[{"id":0,"p":1}],
		[{"id":0,"p":0.5},{"id":1,"p":0.95},{"id":2,"p":1}],
		[{"id":1,"p":0.05},{"id":4,"p":0.15},{"id":2,"p":0.85},{"id":3,"p":1}],
		[{"id":3,"p":1}]
	]
	let random_select_sheule = (age_type_id)=>{
		let r = Math.random();
		let n = 0;
		while(age_shedule_types[age_type_id][n].p<r)n++;
		return age_shedule_types[age_type_id][n];
	}
	let town_array = [];
	var conv_time = (time) => (daylength*time/24)>>>0;
	for(k in population_cnt){
		let N = population_cnt[k];
		while(N--){
			let track = "";
			let lockdown_track = "";
			let lokdown_loop_length = 24*5;
			let rel_time = (Math.random()*lokdown_loop_length)>>>0;
			let closest_magazine = (mc*Math.random())>>>0;
			let house = (hc*Math.random())>>>0;
			if(k>1)
				lockdown_track = `H:${house}:${rel_time}; M:${closest_magazine}:1; H:${house}:${lokdown_loop_length-1-rel_time};`;
			let home_time = conv_time(8 + randn_bm()*1.5);
			let time_left = conv_time(24) - home_time;
			let external_fixed_rnd = Math.random();
			for(let i=0;i<trackregenscnt;i++){
				track += `H:${house}:${home_time}; `
				let last_transp = "";
				var type = random_select_sheule(k);
				var str = "";
				for(val of fixed_shedules[type.id]){
					let len = (val.mid + val.len_dist*randn_bm());
					if(len<=0 || !conv_time(len))
						continue;
					time_left -= conv_time(len);
					let rnd_id = Math.random()*objcnt[val.what];
					if(!val.id_d)
						rnd_id = external_fixed_rnd*objcnt[val.what]
					let loc_str = `${val.what}:${rnd_id>>>0}:${conv_time(len)}`;
					if(val.what=='T')
						last_transp = loc_str;
					track+=loc_str+"; ";
				}
				if(last_transp!="")
					track += last_transp+`; `;
				track += `H:${house}:${time_left}; `
			}
			obj = {};
			obj.age = ages[k];
			obj.track = track;
			obj.special_track = lockdown_track;
			town_array.push(obj);
		}
	}
	return town_array;
}