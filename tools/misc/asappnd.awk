#!/usr/bin/awk -f
# tool to merge assembler outputs

BEGIN {
	heap = 0
	heap_seen = 0
	entry_seen = 0
	data_seen = 0
	failed = 0
}

/^[[:space:]]*heap[[:space:]]+/ {
	if ($2 !~ /^[0-9]+$/) {
		print "asappnd: invalid heap size at " FILENAME ":" FNR > "/dev/stderr"
		failed = 1
		next
	}

	heap += $2
	heap_seen = 1
	next
}

/^[[:space:]]*entry[[:space:]]+/ {
	if (entry_seen) {
		print "asappnd: duplicate entry directive at " FILENAME ":" FNR > "/dev/stderr"
		failed = 1
		next
	}

	entry_seen = 1
}

/^[[:space:]]*\.data[[:space:]]*$/ {
	data_seen = 1
	next
}

{
	is_data = 0

	# Recognize data declarations.
	if ($0 ~ /^[[:space:]]*\.(word|byte|half|long|quad|space|zero|ascii|asciz)([[:space:]]|$)/)
		is_data = 1

	# Recognize labels with inline data declarations.
	if ($0 ~ /^[[:space:]]*[A-Za-z_][A-Za-z0-9_]*[[:space:]]*:[[:space:]]*\.(word|byte|half|long|quad|space|zero|ascii|asciz)([[:space:]]|$)/)
		is_data = 1

	# Check labels in both code and data.
	if (match($0, /^[[:space:]]*[A-Za-z_][A-Za-z0-9_]*[[:space:]]*:/)) {
		label = substr($0, RSTART, RLENGTH)
		sub(/^[[:space:]]*/, "", label)
		sub(/[[:space:]]*:.*/, "", label)

		if (label in labels) {
			print "asappnd: duplicate label '" label \
			    "' at " FILENAME ":" FNR > "/dev/stderr"
			failed = 1
			next
		}

		labels[label] = FILENAME ":" FNR
	}

	if (is_data) {
		data = data $0 "\n"
		data_seen = 1
	} else {
		code = code $0 "\n"
	}
}

END {
	if (failed)
		exit 1

	if (heap_seen)
		print "heap " heap

	printf "%s", code

	if (data_seen) {
		print ".data"
		printf "%s", data
	}
}
