#!/bin/bash
# Run the whole check list over one or more configured build directories:
#
#   script/check_all.sh [-j N] DIR[@MANIFEST] ...
#
# For each DIR: build (and fail on any compiler warning), "make check",
# "make golden-check". Then, once, in the first DIR: "make format-check" and
# "make lint". Prints a summary table, and exits non-zero if anything
# failed. Logs are in each DIR, check_all_<step>.log. The warning check
# only sees what is compiled in this run: for a full check, start from a
# clean build ("make clean" first).
#
# DIR@MANIFEST runs make in DIR through "pixi run --manifest-path MANIFEST"
# (for build directories that each have their own pixi environment); a
# plain DIR runs make in the current environment.
set -u
jobs=16
if [ "${1:-}" = "-j" ]; then
  jobs=$2
  shift 2
fi
if [ $# -eq 0 ]; then
  sed -n '2,16p' "$0" | sed 's/^# \{0,1\}//'
  exit 2
fi

status=0
summary=()

# run_step DIR MANIFEST STEP make-args...: run one make step, log it,
# record the result.
run_step() {
  local dir=$1 manifest=$2 step=$3
  shift 3
  local log="$dir/check_all_$step.log" result
  if [ -n "$manifest" ]; then
    (cd "$dir" && pixi run --manifest-path "$manifest" make "$@") > "$log" 2>&1
  else
    (cd "$dir" && make "$@") > "$log" 2>&1
  fi
  local rc=$?
  if [ $rc -ne 0 ]; then
    result="FAILED (see $log)"
    status=1
  else
    result="ok"
  fi
  if [ "$step" = build ] && [ $rc -eq 0 ]; then
    local nwarn
    nwarn=$(grep -c 'warning:' "$log")
    if [ "$nwarn" -ne 0 ]; then
      result="FAILED: $nwarn warnings (see $log)"
      status=1
    fi
  fi
  if [ "$step" = check ] && [ $rc -eq 0 ]; then
    local py
    py=$(grep -E '[0-9]+ passed' "$log" | tail -1)
    [ -n "$py" ] && result="ok ($py)"
  fi
  summary+=("$(printf '%-40s %-14s %s' "$(basename "$dir")" "$step" "$result")")
}

first=
for arg in "$@"; do
  dir=${arg%%@*}
  manifest=
  [ "$dir" != "$arg" ] && manifest=$(realpath "${arg#*@}")
  dir=$(realpath "$dir")
  [ -z "$first" ] && first="$dir@$manifest"
  run_step "$dir" "$manifest" build -j "$jobs"
  run_step "$dir" "$manifest" check check
  run_step "$dir" "$manifest" golden golden-check
done
dir=${first%%@*}
manifest=${first#*@}
run_step "$dir" "$manifest" format format-check
run_step "$dir" "$manifest" lint lint

echo
printf '%-40s %-14s %s\n' "build directory" "step" "result"
printf '%s\n' "${summary[@]}"
exit $status
