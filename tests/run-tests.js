import JSON5 from 'json5';
import * as fs from 'fs';
import * as child_process from 'child_process';

function runJson5Cpp(path) {
	return JSON.parse(child_process.spawnSync("./json5-to-json", [path]).stdout);
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
	} else if (a == null && isNaN(b)) {
		// The json5 command outputs null for NaN, since NaN is unrepresentable
		// as JSON. Therefore, we consider Json5Cpp null equal to JSON5 NaN.
		return null;
	} else if (a != b) {
		return "A (" + a + ") != B (" + b + ")";
	} else {
		return null;
	}
}

let numTests = 0;
let numSuccesses = 0;

function check(path, impl, implName) {
	numTests += 1;
	process.stderr.write("\r[" + numTests + "] ");
	let fromJson5Cpp;
	try {
		fromJson5Cpp = runJson5Cpp(path);
	} catch (ex) {
		console.log(path + ": failed to parse:", ex);
		return;
	}

	let fromImpl = impl.parse(fs.readFileSync(path));
	let d = diff(fromJson5Cpp, fromImpl);
	if (d != null) {
		console.log(path + ": parse difference:", d);
		console.log("Json5Cpp:", fromJson5Cpp);
		console.log(implName + ".parse:", fromImpl);
		console.log();
		return;
	}

	numSuccesses += 1;
}

function checkJson(path) {
	check(path, JSON, "JSON");
}

function checkJson5(path) {
	check(path, JSON5, "JSON5");
}

for (let entry of fs.readdirSync("json5cpp-tests")) {
	let path = "json5cpp-tests/" + entry;
	checkJson5(path);
}

for (let parent of fs.readdirSync("json5-tests")) {
	let parentPath = "json5-tests/" + parent;
	if (parent[0] == ".") {
		continue;
	}

	// The todo directory seems to contain stuff which isn't in the spec yet
	if (parent == "todo") {
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

console.log("\r" + numSuccesses + "/" + numTests + " tests succeeded.");
if (numSuccesses == numTests) {
	process.exit(0);
} else {
	process.exit(1);
}
