#!/bin/sh
#Copyright (C) 2025-2026 Ivan Gaydardzhiev
#Licensed under the GPL-3.0-only

G='\033[0;32m'
R='\033[0;31m'
C='\033[0;36m'
Y='\033[1;33m'
N='\033[0m'

[ ! -f radix ] && make

fprint() {
	printf "[%s] %-36s %b\n" "$(date '+%Y-%m-%d %H:%M:%S')" "${1}" "${2}"
}

fail_count=0
pass_count=0

run_l1_test() {
	desc="${1}"
	script="${2}"

	expected=$(./radix "${script}")
	capture=$(./radix radix.rx "${script}")

	if [ "${capture}" = "${expected}" ]; then
		fprint "L1: ${desc}" "${G}CONFIRMED${N}"
		pass_count=$((pass_count + 1))
		return 0
	else
		fprint "L1: ${desc}" "${R}REFUTED${N}"
		printf "Expected:\n%s\nGot:\n%s\n" "${expected}" "${capture}"
		fail_count=$((fail_count + 1))
		return 1
	fi
}

run_l2_test() {
	desc="${1}"
	script="${2}"

	expected=$(./radix "${script}")
	capture=$(./radix radix.rx radix.rx "${script}")

	if [ "${capture}" = "${expected}" ]; then
		fprint "L2: ${desc}" "${G}CONFIRMED${N}"
		pass_count=$((pass_count + 1))
		return 0
	else
		fprint "L2: ${desc}" "${R}REFUTED${N}"
		printf "Expected:\n%s\nGot:\n%s\n" "${expected}" "${capture}"
		fail_count=$((fail_count + 1))
		return 1
	fi
}

printf "${C}   Radix Self-Hosting & Metacircular Proof Suite     ${N}\n"

printf "${Y}Level 1 Proofs: (./radix radix.rx <script.rx>)${N}\n"
run_l1_test "Anonymous Functions & Math" "scripts/anon_func.rx"
run_l1_test "Closures & Higher-Order"   "scripts/higher_order_functions_and_closures.rx"
run_l1_test "Tail & Deep Recursion"     "scripts/recursion.rx"
run_l1_test "De Morgan Logical Equiv"   "scripts/demorgan_law.rx"
run_l1_test "Truth Table Verification"  "scripts/truth_table_testing.rx"
run_l1_test "String Primitives & Slices" "scripts/string_test.rx"
run_l1_test "Context-Sensitive Language" "scripts/chomsky_csl.rx"
run_l1_test "Metacircular S-Expression" "scripts/lisp_eval.rx"

./radix radix.rx scripts/kleene_quine.rx > scripts/self_quine.tmp
diff scripts/kleene_quine.rx scripts/self_quine.tmp > /dev/null 2>&1
ret="${?}"
rm -f scripts/self_quine.tmp
if [ "${ret}" -eq 0 ]; then
	fprint "L1: Kleene Fixed-Point Quine" "${G}CONFIRMED${N}"
	pass_count=$((pass_count + 1))
else
	fprint "L1: Kleene Fixed-Point Quine" "${R}REFUTED${N}"
	fail_count=$((fail_count + 1))
fi

printf "\n${Y}Level 2 Proofs: (./radix radix.rx radix.rx <script.rx>)${N}\n"
printf "Executing the interpreter inside the interpreted interpreter...\n"
run_l2_test "Anonymous Functions & Math" "scripts/anon_func.rx"
run_l2_test "Closures & Higher-Order"   "scripts/higher_order_functions_and_closures.rx"
run_l2_test "De Morgan Logical Equiv"   "scripts/demorgan_law.rx"
run_l2_test "Tail & Deep Recursion"     "scripts/recursion.rx"
run_l2_test "String Primitives & Slices" "scripts/string_test.rx"

printf "\n${Y}Level 3 Proof: (./radix radix.rx radix.rx radix.rx <script.rx>)${N}\n"
printf "Executing at recursion depth 3...\n"
exp_l3=$(./radix scripts/anon_func.rx)
cap_l3=$(./radix radix.rx radix.rx radix.rx scripts/anon_func.rx)
if [ "${cap_l3}" = "${exp_l3}" ]; then
	fprint "L3: Arbitrary Depth (Depth 3)" "${G}CONFIRMED${N}"
	pass_count=$((pass_count + 1))
else
	fprint "L3: Arbitrary Depth (Depth 3)" "${R}REFUTED${N}"
	printf "Expected: %s, Got: %s\n" "${exp_l3}" "${cap_l3}"
	fail_count=$((fail_count + 1))
fi

printf "Self-Hosting Summary: ${G}%d Proofs Confirmed${N}, ${R}%d Refuted${N}\n" "${pass_count}" "${fail_count}"

[ "${fail_count}" -eq 0 ] || exit 1
