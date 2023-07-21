import JSON5 from 'json5';
import * as fs from 'fs';
import * as child_process from 'child_process';

function runJson5Cpp(path) {
	return JSON.parse(child_process.spawnSync("./json5", [path]).stdout);
}

function diff(a, b) {
	if (typeof(a) == "object" && typeof(b) == "object") {
		if (a instanceof Array && !(b instanceof Array)) {
			return "A is array, B is object";
		} else if (!(b instanceof Array) && a instanceof Array) {
			return "A is object B is array";
		}

		for (let k in a) {
			if (!a.hasOwnProperty(k)) {
				continue;
			}

			if (!b.hasOwnProperty(k)) {
				return "Key '" + k + "' in A but not in B";
			}

			let d = diff(a[k], b[k]);
			if (d != null) {
				return "." + k + ": " + d;
			}
		}

		for (let k in b) {
			if (!b.hasOwnProperty(a)) {
				continue;
			}

			if (!a.hasOwnProperty(k)) {
				return "Key '" + k + "' in B but not in A";
			}
		}
	} else if (isNaN(a) && isNaN(b)) {
		return null;
	} else if (a != b) {
		return "A (" + a + ") != B (" + b + ")";
	} else {
		return null;
	}
}

let numTests = 0;
let numSuccesses = 0;

function checkJson(path) {
	numTests += 1;
	let fromJson5Cpp;
	try {
		fromJson5Cpp = runJson5Cpp(path);
	} catch (ex) {
		console.log(path + ": failed to parse:", ex);
		return;
	}

	let fromJson = JSON.parse(fs.readFileSync(path));
	let d = diff(fromJson5Cpp, fromJson);
	if (d != null) {
		console.log(path + ": parse difference:", d);
		console.log("Json5Cpp:", JSON.stringify(fromJson5Cpp, null, 4));
		console.log("JSON.parse:", JSON.stringify(fromJson, null, 4));
		console.log();
		return;
	}

	numSuccesses += 1;
}

function checkJson5(path) {
	numTests += 1;
	let fromJson5Cpp;
	try {
		fromJson5Cpp = runJson5Cpp(path);
	} catch (ex) {
		console.log(path + ": failed to parse:", ex);
		return;
	}

	let fromJson5 = JSON5.parse(fs.readFileSync(path));
	let d = diff(fromJson5Cpp, fromJson5);
	if (d != null) {
		console.log(path + ": parse difference:", d);
		console.log("Json5Cpp:", JSON.stringify(fromJson5Cpp, null, 4));
		console.log("JSON5.parse:", JSON.stringify(fromJson5, null, 4));
		console.log();
		return;
	}

	numSuccesses += 1;
}

for (let parent of fs.readdirSync("json5-tests")) {
	let parentPath = "json5-tests/" + parent;
	if (parent[0] == ".") {
		continue;
	}

	let stat = fs.statSync(parentPath);
	if (!stat.isDirectory()) {
		continue;
	}

	for (let entry of fs.readdirSync(parentPath)) {
		let path = parentPath + "/" + entry;
		if (path.endsWith(".json")) {
			checkJson(path);
		} else if (path.endsWith(".json5")) {
			checkJson5(path);
		}
	}
}

for (let entry of fs.readdirSync("JSONTestSuite/test_parsing")) {
	let path = "JSONTestSuite/test_parsing/" + entry;
	if (entry[0] == "y") {
		checkJson(path);
	}
}

console.log(numSuccesses + "/" + numTests + " tests succeeded.");
if (numSuccesses == numTests) {
	process.exit(0);
} else {
	process.exit(1);
}
