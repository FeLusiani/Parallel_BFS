# parses output of execute.sh and collects execution times

from pathlib import Path
import numpy as np

result_str = Path(sys.argv[1]).read_text()

lines = result_str.split('\n')


measures = {}
current_n_workers = 0

for l in lines:
    if l.startswith('Running each BFS for')
        current_n_workers = int(l.split()[4])

    if l.startswith('Exec times for')
        tokens = l.split()
        alg_name = tokens[3]
        times = [int(t) for t in tokens[5:]]
        measures[current_n_workers][alg_name+'_mean'] = np.mean(times)
        measures[current_n_workers][alg_name+'_var'] = np.std(times)

print(measures)


