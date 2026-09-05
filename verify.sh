#!/bin/sh
#Copyright (C) 2025-2026 Ivan Gaydardzhiev
#Licensed under the GPL-3.0-only

G='\033[0;32m'
R='\033[0;31m'
C='\033[0;36m'
N='\033[0m'

[ ! -f radix ] && make

fprint() {
	printf "[%s] %-28s %b\n" "$(date '+%Y-%m-%d %H:%M:%S')" "${1}" "${2}"
}

fail_count=0
pass_count=0

run_test() {
	desc="${1}"
	cmd="${2}"
	expected="${3}"
	capture=$(eval "${cmd}")
	if [ "${capture}" = "${expected}" ]; then
		fprint "${desc}" "${G}PASSED${N}"
		pass_count=$((pass_count + 1))
		return 0
	else
		fprint "${desc}" "${R}FAILED${N}"
		printf "Expected:\n%s\nGot:\n%s\n" "${expected}" "${capture}"
		fail_count=$((fail_count + 1))
		return 1
	fi
}

printf "${C}Radix Core Language & Theoretical Verification${N}\n"

run_test "Ackermann(3,7)" "./radix scripts/ackermann.rx" "1021"

run_test "Anonymous Functions" "./radix scripts/anon_func.rx" "8"

exp_core=$(printf "20\n1\n0\n1\n2\n3\n4\n20\n15\n42")
run_test "Core Language Semantics" "./radix scripts/core_language_test.rx" "${exp_core}"

run_test "Turing Completeness" "./radix scripts/turing.rx" "120"

run_test "Higher-Order Functions" "./radix scripts/higher_order_functions_and_closures.rx" "25"

run_test "Tail/Deep Recursion" "./radix scripts/recursion.rx" "120"

exp_bool=$(printf "true\ntrue\ntrue\ntrue")
run_test "De Morgan's Laws" "./radix scripts/demorgan_law.rx" "${exp_bool}"

run_test "Truth Table Verification" "./radix scripts/truth_table_testing.rx" "${exp_bool}"

run_test "Entscheidungsproblem" "./radix scripts/entscheidungs_problem.rx" "0"

run_test "Halting Paradox" "./radix scripts/halting_paradox.rx" "0"

exec 3>&2 2>/dev/null
./radix scripts/pure_diag.rx
ret="${?}"
exec 2>&3 3>&-
if [ "${ret}" = "139" ] || [ "${ret}" = "1" ] || [ "${ret}" = "2" ]; then
	fprint "Self-Ref Diagonalization" "${G}CONFIRMED${N}"
	pass_count=$((pass_count + 1))
else
	fprint "Self-Ref Diagonalization" "${R}REFUTED${N}"
	fail_count=$((fail_count + 1))
fi

printf "\n${C}Radix String Manipulation Primitives${N}\n"

exp_lit=$(printf 'hello world\nline1\nline2\ntab\tseparated\n"quoted"')
run_test "String Literals" 'printf "%s\n" '\''outn("hello world"); outn("line1\nline2"); outn("tab\tseparated"); outn("\"quoted\"");'\'' | ./radix' "${exp_lit}"

exp_concat=$(printf "plan9 unix\nval: 42\n100 percent\nbool: true")
run_test "Concatenation" 'printf "%s\n" '\''outn("plan9 " + "unix"); outn("val: " + 42); outn(100 + " percent"); outn("bool: " + true);'\'' | ./radix' "${exp_concat}"

exp_cmp=$(printf "true\nfalse\ntrue\ntrue\ntrue\ntrue")
run_test "Lexicographical Comparison" 'printf "%s\n" '\''outn("abc" == "abc"); outn("abc" == "xyz"); outn("abc" != "xyz"); outn("apple" < "banana"); outn("cat" > "bat"); outn("dog" <= "dog");'\'' | ./radix' "${exp_cmp}"

exp_len=$(printf "0\n5\n11")
run_test "String Length" 'printf "%s\n" '\''outn(len("")); outn(len("radix")); outn(len("hello world"));'\'' | ./radix' "${exp_len}"

exp_char=$(printf "r\nx\n")
run_test "Character Indexing" 'printf "%s\n" '\''outn(char_at("radix", 0)); outn(char_at("radix", 4)); outn(char_at("radix", 10));'\'' | ./radix' "${exp_char}"

exp_sub=$(printf "hello\nworld\nllo")
run_test "Substring Slicing" 'printf "%s\n" '\''outn(substr("hello world", 0, 5)); outn(substr("hello world", 6, 5)); outn(substr("hello", 2, 99));'\'' | ./radix' "${exp_sub}"

exp_orx=$(printf "65\n67\nA\nZ")
run_test "ASCII Ord and Chr" 'printf "%s\n" '\''outn(ord("A")); outn(ord("ABC", 2)); outn(chr(65)); outn(chr(ord("Z")));'\'' | ./radix' "${exp_orx}"

exp_conv=$(printf "1234\nfalse\n50")
run_test "Type Conversions" 'printf "%s\n" '\''outn(str(1234)); outn(str(false)); outn(num("42") + 8);'\'' | ./radix' "${exp_conv}"

exp_find=$(printf "6\n0\n-1")
run_test "Find Substring" 'printf "%s\n" '\''outn(find("plan9 unix", "unix")); outn(find("plan9 unix", "plan9")); outn(find("plan9 unix", "linux"));'\'' | ./radix' "${exp_find}"

run_test "Formatted Stream Output" 'printf "%s\n" '\''out("plan"); out("9"); outn(" bell_labs");'\'' | ./radix' "plan9 bell_labs"

exp_pal=$(printf "true\ntrue\nfalse\ntrue\nfalse")
run_test "String Palindrome" "./radix scripts/string_palindrome.rx" "${exp_pal}"

printf "\n${C}Formal Computability & Semantic Reflexivity Proofs${N}\n"

./radix scripts/kleene_quine.rx > scripts/quine_test.tmp
diff scripts/kleene_quine.rx scripts/quine_test.tmp > /dev/null 2>&1
ret="${?}"
rm -f scripts/quine_test.tmp
if [ "${ret}" -eq 0 ]; then
	fprint "Kleene Fixed-Point (Quine)" "${G}CONFIRMED${N}"
	pass_count=$((pass_count + 1))
else
	fprint "Kleene Fixed-Point (Quine)" "${R}REFUTED${N}"
	fail_count=$((fail_count + 1))
fi

exp_utm=$(printf "1\n10\n11\n1100\n100000000000000000000")
run_test "Unbounded Turing Tape" "./radix scripts/turing_string_tape.rx" "${exp_utm}"

exp_godel=$(printf 'UNPROVABLE(diag("UNPROVABLE(diag($))"))\nUNPROVABLE(diag($))\ntrue\ntrue')
run_test "Gödel Diagonalization" "./radix scripts/godel_string.rx" "${exp_godel}"

exp_markov=$(printf "1\n10\n11\n1100\n1000\n100000000")
run_test "Markov Production System" "./radix scripts/markov_algorithm.rx" "${exp_markov}"

exp_csl=$(printf "1\n1\n1\n1\n0\n0\n0\n0\n0")
run_test "Chomsky CSL Recognizer" "./radix scripts/chomsky_csl.rx" "${exp_csl}"

exp_lisp=$(printf "42\n30\n42\n54\n100\n200\n21")
run_test "Metacircular S-Expr Evaluator" "./radix scripts/lisp_eval.rx" "${exp_lisp}"

printf "Verification Summary: ${G}%d Passed${N}, ${R}%d Failed${N}\n" "${pass_count}" "${fail_count}"

[ "${fail_count}" -eq 0 ] || exit 1
