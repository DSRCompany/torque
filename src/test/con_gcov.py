#! /usr/bin/env python

# concatenate gcov output files

import os, re, sys

if __name__ == '__main__':
    files = []
    if len(sys.argv[1:]) == 0:
        sys.argv.append("-h")
    for key in sys.argv[1:]:
        if (key == '--help' or key == '-h'):
            print('con_gcov [--help|-h] [-f\"gcov_file1\"] [-f\"gcov_file2\"] ... [-f\"gcov_fileN\"] ')
            print('   --help, -h - print help page')
            print('   -f\"gcov_file\" - gcov result file')
            sys.exit(0)
        elif (key[:2] == '-f'):
            if key[2:]:
                files.append(key[2:])

    if files:
        src_map = {}
        for file in files:
            with open(file, 'r') as f:
                lines = f.readlines()
                for line in lines:
                    if line[:10] != "        -:" and line[10:15] != "    0":
                        value = 0
                        line_num = 0
                        if line[:9] != "    #####":
                            value = int(line[:9])
                        line_num = int(line[10:15])
                        if line_num not in src_map:
                            src_map[line_num] = value
                        else:
                            src_map[line_num] = src_map[line_num] + value
        void_str = 0
        for it in src_map:
            if src_map[it] == 0:
                void_str = void_str + 1;

        filename = os.path.basename(files[0][:-5])
        #print "result: " + str(100 - ((100 * void_str)/len(src_map))) + "%"
        result_percent = 100.0 - ((100.0 * void_str)/len(src_map))
        #print "\033[1;33mTOTALCOV -- " + filename + ": Lines(" + str(len(src_map)) + ") - executed:" + str(result_percent) + "%\033[0m"
        print "\033[1;33mTOTALCOV -- %s: Lines(%d) - executed:%0.2f%%\033[0m" % (filename, len(src_map), result_percent)
