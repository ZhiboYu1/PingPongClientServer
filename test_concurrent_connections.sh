#!/bin/bash


SERVER_HOST="agate"
SERVER_PORT=18000

# Helper to run N clients with given args
run_clients() {
  local count=$1
  shift
  echo ">>> Running $count clients with args: $*"
  for idx in $(seq 1 $count); do
    ./client "$SERVER_HOST" "$SERVER_PORT" "$@" &
  done
  wait
  echo ">>> Finished $count clients"
  echo
}

### TEST CASES ###

# Case 1: Small number of clients (baseline)
run_clients 3 24 50

# Case 2: Medium number of clients
run_clients 10 24 50

# Case 3: Large number of clients (stress test)
run_clients 50 24 50

# Case 4: Different request sizes
run_clients 5 10 20
run_clients 5 100 200

# Case 5: Mixed scenarios (sequential)
run_clients 5 24 50
run_clients 5 48 100

# Case 6: Rapid bursts of clients
for round in {1..3}; do
  run_clients 10 24 50
done