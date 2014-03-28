/*
 *
 * Reads a CSV file, searches the first line for fields
 * called heartrate, distance and time and iterates over 
 * every row calculating the pace and hr averages for the 
 * corresponding target heartrate and target pace specified
 * using the options -h and -p.
 *
 */


#include <getopt.h>
#include <get_goals_rubi_csv.h>
#include <chs_debug.h>

void showHelp(const struct option * long_options, const char * name) {
    fprintf(stderr, "Parameters missing, use:\n%s [options] <-h <target HR>> <-p <target pace>>  <file>\n", name);
    for (int opt = 0;; opt++) {
        if (long_options[opt].name != NULL) {
            fprintf(stderr, "\t--%s", long_options[opt].name);
            if (long_options[opt].has_arg == required_argument) {
                fprintf(stderr, "|-%c\t\t<argument>\n", (char) long_options[opt].val);
            } else {
                fprintf(stderr, "\n");
            }
        } else break;
    }
}

int processArguments(const int * argc,
        char * const * argv,
        const struct option * long_options,
        FILE * fd_out,
        int * target_hr1,
        int * target_hr2,
        Pace * t_pace1,
        Pace * t_pace2) {
    int c;
    while (1) {

        /* getopt_long stores the option index here. */
        int option_index = 0;

        c = getopt_long(*argc, argv, "h:p:o:",
                long_options, &option_index);

        /* Detect the end of the options. */
        if (c == -1)
            break;

        switch (c) {
            case 0:
                /* If this option set a flag, do nothing else now. */
                if (long_options[option_index].flag != 0)
                    break;
                printf("option %s", long_options[option_index].name);
                if (optarg)
                    printf(" with arg %s", optarg);
                printf("\n");
                break;

            case 't':
                puts("option -t\n");
                break;

            case 'h':
                verbose("option -h has value '%s'\n", optarg);
                if ((*target_hr1 = validate_str_HR(optarg)) == 0) {
                    fprintf(stderr, "Bad parameter <HR>\n");
                    return (-1);
                }
                break;

            case 'H':
                break;

            case 'p':
                // fprintf (stderr, "option -p with value `%s'\n", optarg);
                if (validate_str_time(optarg, t_pace1) <= 0) {
                    fprintf(stderr, "Bad parameter <PACE(mm:ss.ds)>\n");
                    return (-1);
                }
                break;

            case 'P':
                break;

            case 'o':

                fd_out = fopen(optarg, "w");
                if (fd_out == NULL) {
                    fprintf(stderr, "Could not open %s (errno=%d)", optarg, errno);
                } else {
                    fprintf(stderr, "Writing output to %s\n", optarg);
                }

                break;

            case '?':
                /* getopt_long already printed an error message. */
                break;

            default:
                abort();
        }
    }

    if (verbose_flag)
        puts("Verbosity is on.");

    return (0);
}

size_t load_file_bytes(const char * filename, char ** file_bytes ) {
    FILE *fd;
    size_t file_size = 0;
    
    fd = fopen(filename, "r");

    if (fd == NULL) {
        fprintf(stderr, "Cannot open file: %s (errno=%i)\n", filename, errno);
        switch (errno) {
            case ENOENT:
                fprintf(stderr, "File not found\n");
                break;
        }
        return (-1);
    } else {
        fseek(fd, 0, SEEK_END);
        file_size = ftell(fd);
        fseek(fd, 0, SEEK_SET);

        if (file_size > 0)
            *file_bytes = (char *) malloc(sizeof (char)*file_size);
        else {
            fprintf(stderr, "File %s has 0 bytes\n", filename);
        }
    }

    if (*file_bytes != NULL) {
        file_size = fread(*file_bytes, sizeof (char), file_size, fd);
        if (fclose(fd) != 0) {
            fprintf(stderr, "Error closing file (errno = %d)\n", errno);
            return -1;
        }
    }
    return file_size;
}

int main(int argc, char ** argv) {
    FILE *fd_out;
    Pace pace;
    Pace pace_at_hr = {0, 0, 0};
    Pace total_pace_at_hr = {0, 0, 0};
    char *file_bytes;
    float hr_at_pace = 0;
    float sum_of_hrs = 0;
    float sum_of_paces = 0;

    int nr_of_files = 0;
    int target_hr;
    size_t file_size = 0;
    int p_lines = 0;
    int h_lines = 0;

    int l_count, l_test;
    Sample head;
    // TODO: Add menu of Kernel generators as a parameter
    float window[] = {0.010333864010783912, 0.20756120714779008, 0.564209857682852, 0.20756120714779008, 0.010333864010783912}; //Norm sigma = 0.5
    int window_len = 5;

    float f_pace;

    static struct option long_options[] = {
        /* These options set a flag. */
        {"verbose", no_argument, &verbose_flag, 1},
        {"brief", no_argument, &verbose_flag, 0},
        /* These options don't set a flag.
         We distinguish them by their indices. */
        {"heartrate", required_argument, 0, 'h'},
        {"heartrate-range", required_argument, 0, 'H'},
        {"pace", required_argument, 0, 'p'},
        {"pace-range", required_argument, 0, 'P'},
        {"out", required_argument, 0, 'o'},
        {"test", required_argument, 0, 't'},
        {0, 0, 0, 0}
    };

    fd_out = stdout;

    processArguments(&argc, argv, long_options, fd_out, &target_hr, 0, &pace, 0);

    if (optind < argc) {
        verbose("Target HR: %d\n", target_hr);
        fprintf(stderr, "Target Pace(%s): %s\n", (validate_str_time(argv[2], &pace) > 0 ? "OK" : "NOK"), pace_to_str(&pace));

        fprintf(fd_out, "HR at\t\tPace at\t\tFile\n");
        fprintf(fd_out, "%s\t\t%d\t\tName\n", pace_to_str(&pace), target_hr);
        fprintf(fd_out, "----------------------------------------------\n");

        // "non-option ARGV-elements: "
        while (optind < argc) {
            nr_of_files++;

            file_size = load_file_bytes(argv[optind], &file_bytes);
            if (file_size <= 0){
                nr_of_files--; // If we can't open a file, we skip counting it!
                continue;
            }
            
            l_count = 0;
            l_test = loadValues(&l_count, &head, file_bytes, &file_size);
            verbose("l_test: %d\tl_count: %d\n", l_test, l_count);

            // TODO: Make this optional
            lowPassHR(&head, window, &window_len);

            // TODO: Pass tolerance as parameter or a a range from command line
            p_lines = average_pace_at_HR(&head, &target_hr, 1, &f_pace);
            float_to_Pace(&pace_at_hr, f_pace);

            // TODO: Pass Tolerance factor as a parameter
            h_lines = average_hr_at_Pace(&head, &pace, 0.01, &hr_at_pace);

            // TODO: Review this validation... it used to be related to a parsing error...
            if (p_lines > 0) {
                fprintf(fd_out, "%.1f(%d)\t%s(%d)\t%s(%dK, %d Records)\n", hr_at_pace, h_lines, pace_to_str(&pace_at_hr), p_lines, argv[optind], (int) file_size / 1024, l_count);
                sum_of_hrs += hr_at_pace;
                sum_of_paces += f_pace;
            } else {
                fprintf(stderr, "Error processing file %s (errno = %d)\n", argv[optind], errno);
                if (errno == EINVAL) {
                    fprintf(stderr, "\tIncorrect or incompatible first line. No usable fields found.\n");
                }
                nr_of_files--; // If we can't open a file, we skip it!!
            }

            // TODO: Enable/Disable this line with a "Cumulative" Switch.
            head = *((Sample *) malloc(sizeof (Sample)));
            free(file_bytes);
            optind++;
        }

        float_to_Pace(&total_pace_at_hr, sum_of_paces / (float) nr_of_files);

        fprintf(fd_out, "----------------------------------------------\n");
        fprintf(fd_out,
                "%.1f\t%s\t%d files\n",
                (sum_of_hrs / (float) nr_of_files), pace_to_str(&total_pace_at_hr), nr_of_files);
        if (fd_out != stdout) {
            fflush(fd_out);
            fclose(fd_out);
        }

    } else {
        showHelp(long_options, argv[0]);
        return (-1);
    }
    exit(0);
}



