// d7ca763 prototype table projection, restricted to the published initial autonomous interval.
// Inputs have already passed compile_startup's byte/provenance checks; still validate all shapes.
export function compileInitialAi(entries, first, facilityRows) {
  const need = (ok, why) => { if (!ok) throw new Error(why); };
  const decimal = value => {
    need(typeof value === 'string' && /^-?\d+$/.test(value), 'Invalid AI table integer');
    const n = Number(value);
    need(Number.isSafeInteger(n) && n >= -2147483648 && n <= 2147483647, 'AI integer overflow');
    return n;
  };
  const numbers = value => value === '' ? [] : value.split('&').map(decimal);
  const list = values => `{${values.join(',')}}`;
  const jobs = entries.get('job.txt'), weapons = entries.get('weapon.txt');
  need(jobs?.length === 23 && weapons?.length === 33, 'Incomplete AI profession/equipment tables');
  jobs.forEach((row, index) => {
    need(row.length === 24 && decimal(row[0]) === index && numbers(row[9]).length === 6 &&
      numbers(row[10]).length === 6 && numbers(row[9]).every(v => v >= 0) &&
      numbers(row[10]).every(v => v >= 0) && decimal(row[20]) >= 1 && decimal(row[20]) <= 5,
      'Invalid AI profession rule');
  });
  weapons.forEach((row, index) => need(row.length === 19 && decimal(row[0]) === index &&
    decimal(row[5]) >= 0 && decimal(row[11]) >= 0, 'Invalid AI weapon rule'));
  const person = entries.get('character.txt')?.find(row => decimal(row[0]) === first.definition_id);
  need(person?.length >= 14 && decimal(person[3]) === first.job_id && numbers(person[12]).length === 6,
    'Invalid AI first definition');
  const levels = Array(jobs.length).fill(1), initialJobs = numbers(person[4]), initialLevels = numbers(person[5]);
  need(initialJobs.length === initialLevels.length, 'Invalid initial profession level arrays');
  initialJobs.forEach((job, n) => {
    need(job >= 0 && job < jobs.length && initialLevels[n] >= 1 && initialLevels[n] <= 10,
      'Invalid initial profession level');
    levels[job] = initialLevels[n];
  });
  need(Number.isInteger(first.job_id) && first.job_id >= 0 && first.job_id < jobs.length,
    'Invalid first profession');
  const weapon = weapons[first.equipment_ids[0]], job = jobs[first.job_id];
  need(weapon && first.equipment_ids.slice(1).every(id => id === -1), 'Unsupported initial equipment');
  const spells = Array.from({length: 4}, (_, slot) => jobs.find(row =>
    decimal(row[18]) >= 10 && decimal(row[18]) % 10 === slot)).filter(Boolean).map(row => decimal(row[0]));
  const services = [28, 30, 33, 35, 45].map(id => {
    const row = facilityRows.get(id);
    need(row?.length === 36, 'Missing initial AI service');
    const attrs = numbers(row[28]), deltas = numbers(row[29]);
    need(attrs.length === deltas.length && attrs.every(v => v >= 0 && v < 6) && decimal(row[25]) >= 0,
      'Invalid initial AI wait/attribute effects');
    return `{${id},{${decimal(row[25])},{${attrs.map((v, n) => `{${v},${deltas[n]}}`).join(',')}}}}`;
  });
  // Source initial equip explicitly installs A[0]=6; it is not a cost or waiting duration.
  return `const InitialAiRules &initial_ai_rules() { static const InitialAiRules value{\n` +
    `{${first.job_id},${decimal(person[6])},${list(numbers(person[12]))},{},${list(levels)},` +
    `{{std::array<int,4>${list(weapon.slice(12,16).map(decimal))},std::nullopt,std::nullopt,std::nullopt}},` +
    `{},${list(spells)}},\n` +
    `{${jobs.map(row => `{${list(numbers(row[9]))},${list(numbers(row[10]))},${decimal(row[20])},${(decimal(row[23]) & 1) !== 0}}`).join(',')}},\n` +
    `{${weapons.map(row => `{{${decimal(row[0])},${decimal(row[5])},${(decimal(row[18]) & 1) !== 0}},${decimal(row[11])},${list(row.slice(12,16).map(decimal))}}`).join(',')}},\n` +
    `${list(job.slice(13,15).map(decimal))},6,{${services.join(',')}}}; return value; }\n`;
}
