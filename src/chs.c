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

/* Flag set by ‘--verbose’. */
static int verbose_flag;



int main (int argc , char ** argv){
    FILE *fd, *fd_out;
    Pace pace;
	Pace pace_at_hr = {0,0,0};
    Pace total_pace_at_hr = {0,0,0};
	char *file_bytes;
	float hr_at_pace = 0;
    float sum_of_hrs = 0;
    float sum_of_paces = 0;
    int c;
    int nr_of_files = 0;
    int target_hr;
        size_t file_size = 0;
        int lines = 0;
    
    fd_out = stdout;
    
    while (1)
    {
        static struct option long_options[] =
        {
            /* These options set a flag. */
            {"verbose", no_argument,       &verbose_flag, 1},
            {"brief",   no_argument,       &verbose_flag, 0},
            /* These options don't set a flag.
             We distinguish them by their indices. */
            {"heartrate",   required_argument, 0, 'h'},
            {"pace",        required_argument, 0, 'p'},
            {"out",         required_argument, 0, 'o'},
            {"test",         required_argument, 0, 't'},
            {0, 0, 0, 0}
        };
        /* getopt_long stores the option index here. */
        int option_index = 0;
        
        c = getopt_long (argc, argv, "h:p:o:",
                         long_options, &option_index);
        
        /* Detect the end of the options. */
        if (c == -1)
            break;
        
        switch (c)
        {
            case 0:
                /* If this option set a flag, do nothing else now. */
                if (long_options[option_index].flag != 0)
                    break;
                printf ("option %s", long_options[option_index].name);
                if (optarg)
                    printf (" with arg %s", optarg);
                printf ("\n");
                break;
                
            case 't':
                puts ("option -t\n");
                break;
                
            case 'h':
                // fprintf (stderr, "option -h with value `%s'\n", optarg);
                if((target_hr=validate_str_HR(optarg)) == 0){
                    fprintf(stderr, "Bad parameter <HR>\n");
                    return(-1);
                }
                break;
            
            case 'p':
                // fprintf (stderr, "option -p with value `%s'\n", optarg);
                if( validate_str_time(optarg, &pace)<=0){
                    fprintf(stderr, "Bad parameter <PACE(mm:ss.ds)>\n");
                    return(-1);
                }
                break;
                
            case 'o':
                
                fd_out = fopen(optarg, "w");
                if (fd_out == NULL){
                    fprintf(stderr, "Could not open %s (errno=%d)", optarg, errno);
                }else{
                    fprintf (stderr, "Writing output to %s\n", optarg);
                }
                
                break;
                
            case '?':
                /* getopt_long already printed an error message. */
                break;
                
            default:
                abort ();
        }
    }
    
    if (verbose_flag)
        puts ("verbose flag is set");
    
    /* Print any remaining command line arguments (not options). */
    if (optind < argc)
    {
        fprintf(stderr, "Target HR: %d\n", target_hr);
        fprintf(stderr, "Target Pace(%s): %s\n", (validate_str_time(argv[2], &pace)>0?"OK":"NOK"), pace_to_str(&pace));
        
        fprintf(fd_out, "HR at\tPace at\tFile\n");
        fprintf(fd_out, "%s\t%d\tName\n", pace_to_str(&pace), target_hr);
        fprintf(fd_out, "-----------------------\n");
        
        // "non-option ARGV-elements: "
        while (optind < argc){
            nr_of_files++;
            
            fd = fopen(argv[optind], "r");
            
            if (fd == NULL){
                fprintf(stderr, "Cannot open file: %s (errno=%i)\n", argv[optind], errno);
                switch(errno){
                    case ENOENT:
                        fprintf(stderr, "File not found\n");
                        break;
                }
                return (-1);
            }else{
                fseek(fd, 0, SEEK_END);
                file_size = ftell(fd);
                fseek(fd, 0, SEEK_SET);
                
                if(file_size > 0)
                    file_bytes = (char *)malloc(sizeof(char)*file_size);
                else{
                    fprintf(stderr, "File %s has 0 bytes\n", argv[optind]);
                }
            }
            
            if (file_bytes != NULL){
                file_size = fread(file_bytes, sizeof(char), file_size, fd );
                if (fclose(fd) != 0){
                    fprintf(stderr, "Error closing file (errno = %d)\n", errno);
                    nr_of_files--; // If we can't open a file, we skip it!
                }
            }
            
            // fprintf(stderr, "%lu bytes read from %s\n", file_size, argv[f_index]);
            
            if(computeValues(&lines, &hr_at_pace, &pace_at_hr, file_bytes, &file_size, &target_hr, &pace) > 0){
                fprintf(fd_out, "%.1f\t%s\t%s(%dK, %d Records)\n",  hr_at_pace,pace_to_str(&pace_at_hr), argv[optind], (int)file_size/1024, lines);
                sum_of_hrs += hr_at_pace;
                sum_of_paces += pace_to_float(&pace_at_hr);
            }else{
                fprintf(stderr, "Error processing file %s (errno = %d)\n", argv[optind], errno);
                if (errno == EINVAL){
                    fprintf(stderr, "\tIncorrect or incompatible first line. No usable fields found.\n");
                }
                nr_of_files--; // If we can't open a file, we skip it!
            }
            
            free(file_bytes);
            optind++;
        }
        
        float_to_Pace(&total_pace_at_hr, sum_of_paces/(float)nr_of_files);
        
        fprintf(fd_out, "-----------------------\n");
        fprintf(fd_out,
                "%.1f\t%s\t%d files\n",
                (sum_of_hrs/(float)nr_of_files),pace_to_str(&total_pace_at_hr), nr_of_files);
        if(fd_out != stdout){
            fflush(fd_out);
            fclose(fd_out);
        }
        
    } else{
        fprintf(stderr, "Parameters missing, use: <HR> <PACE(mm:ss.ds)> <file>\n");
		return(-1);
    }
    
    exit (0);
}



