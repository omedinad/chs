#include <get_goals_rubi_csv.h>

#define MAX_HR 190
#define MIN_HR 45
#define HR_TOLERANCE 3 // Bpm
#define PACE_TOLERANCE 10 // Seconds
#define HDR_HR "heartrate"
#define HDR_DIST "dist"
#define HDR_TIME "time"
#define CHECK_FOR_PACE_OUTLIERS 1

int validate_int_HR(int hr) {
    return ((hr < MAX_HR)&&(hr > MIN_HR) ? hr : 0);
}

int validate_str_HR(char * strhr) {
    int hr = 0;
    if (strhr != NULL) {
        hr = atoi(strhr);
        return validate_int_HR(hr);
    } else {
        return hr;
    }
}

int validate_str_time(const char * tstring, Pace *p) {
    int min;
    int sec;
    int dec;

    sscanf(tstring, "%d:%d.%d", &min, &sec, &dec);

    if (min > 0 && min < 60) p->min = min;
    else return -1;
    if (sec > 0 && sec < 60) p->sec = sec;
    else return -1;
    if (dec >= 0) p->dec = dec;
    else return -1;

    return 1;
}

char * pace_to_str(Pace * pace) {
    char * result;
    result = (char *) malloc(sizeof (char)*10);
    snprintf(result, 10, "%d:%02d.%d", pace->min, pace->sec, pace->dec);
    return result;
}

int get_field_number(const char * field_name, const char * buffer, int len) {
    int llen = -1;
    char * line;
    char * token;
    for (llen = 0; llen < len; llen++)
        if (buffer[llen] == '\n') break;


    line = (char *) malloc(llen);
    memcpy(line, buffer, llen);
    line[llen] = '\0';

    for (token = strtok(line, ","), llen = 0; token; token = strtok(NULL, ","), llen++) {
        if (strcmp(token, field_name) == 0) {
            return llen;
        }
    }

    //printf("%s (line lenght %d)\n", line, llen);

    return -1;
}

void float_to_Pace(Pace * pace, float fpace) {
    pace->min = (int) fpace;
    pace->sec = (int) ((fpace - (float) pace->min) * (float) 60);
}

float pace_to_float(const Pace * pace) {
    return (float) pace->min + ((float) pace->sec / 60);
}

int loadValues(int *count, Sample * head, char * bytes, const size_t * len) {

    int hr_f_nr, dist_f_nr, time_f_nr;

    char * line_t, * field_t, * field_tr;
    int line_nr, field_nr, hr, time;
    float temp_dist, dist[2], dx_dist, speed, dx_pace;
    dist[0] = 0;
    dist[1] = 0;

    Sample * current_sample, * previous_sample;

    hr_f_nr = get_field_number(HDR_HR, bytes, *len);
    // fprintf(stderr, "HR Column nr: %d\n", hr_f_nr);

    dist_f_nr = get_field_number(HDR_DIST, bytes, *len);
    // fprintf(stderr, "Distance Column nr: %d\n", dist_f_nr);

    time_f_nr = get_field_number(HDR_TIME, bytes, *len);
    // fprintf(stderr, "Time Column nr: %d\n", time_f_nr);

    if (hr_f_nr >= 0 && dist_f_nr >= 0 && time_f_nr >= 0) {
        errno = 0;
    } else {
        errno = EINVAL;
        return -1;
    }

    if (head == NULL) {
        verbose("Allocating Head: ");
        head = malloc(sizeof (Sample));
        verbose("%#010x\n", head);
    }

    if (head != NULL) {
        current_sample = head;
        while (current_sample->next != NULL) {
            current_sample = current_sample->next;
            verbose("Searching for tail.\n");
        }
    } else {
        return -1;
    }

    line_t = strtok(bytes, "\n");
    for (line_t = strtok(NULL, "\n"), line_nr = 1; line_t; line_t = strtok(NULL, "\n"), line_nr++) {
        // fprintf(stderr, "%d)\t%s\n", line_nr, line_t);
        assert(current_sample != NULL);

        for (field_t = strtok_r(line_t, ",", &field_tr), field_nr = 0; field_t; field_t = strtok_r(NULL, ",", &field_tr), field_nr++) {
            if (field_nr == hr_f_nr) {
                hr = validate_str_HR(field_t);
            }

            if (field_nr == dist_f_nr) {
                temp_dist = dist[0];
                dist[0] = atof(field_t);
                dist[1] = temp_dist;
            }

            if (field_nr == time_f_nr) {
                time = atoi(field_t);
            }

        }

        /* General Statistics */

        dx_dist = dist[0] - dist[1];
        speed = (dx_dist != 0 ? ((dx_dist / time)*3600) / 1000 : 0); // km/h
        dx_pace = (speed != 0 ? 60 / speed : 0);

        // verbose("%d\t%.2f\t%d\t%.2f Km/h\t%.2f m/Km\n", hr, dx_dist, time, speed , dx_pace);

        /* Collect Samples */
        if (current_sample != NULL) {
            /* Only if current Sample is allocated, we set the values... */
            current_sample->distance = dx_dist;
            current_sample->hr = (float) hr;
            current_sample->pace_min = dx_pace;
            current_sample->speed = speed;
            current_sample->time = time;

            // verbose("%.2f\t%.2f\t%d\t%.2f Km/h\t%.2f m/Km\n", current_sample->hr, current_sample->distance, current_sample->time, current_sample->speed , current_sample->pace_min);

            previous_sample = current_sample;
            current_sample->next = (Sample *) malloc(sizeof (Sample));
            current_sample = current_sample->next;
            current_sample->prev = previous_sample;

            // verbose("p: %#010x\t c: %#010x\t n: %#010x\n", current_sample->prev, current_sample, current_sample->next);

        } else {
            verbose("Current Sample == NULL\n");
            return -1;
        }
    }
    *count = line_nr;
    return 0;
}

int validateWindow(const float * window, const int * len) {
    float sum = 0;
    for (int i = 0; i < *len; i++)
        sum += window[i];
    return (sum == 1 ? 1 : 0);
}

int lowPassHR(Sample * head, const float * window, const int * len) {
    Sample * current_sample, *testL;
    int half_len;
    int left = 0, right = 0;
    float sum;
    verbose("Low pass start\n");
    if (head != NULL && window != NULL) {
        if (*len % 2 != 0) {
            if (validateWindow(window, len) == 1) {
                current_sample = head;
                half_len = floor(*len / 2);
                // Go Half way forward to have a full window.
                for (int i = 0; i <= half_len; i++) {
                    if (current_sample->next != NULL) {
                        current_sample = current_sample->next;
                    } else {
                        verbose("Too few samples (%d so far) or too large window size of %d\n", i, *len);
                    }
                }
                /* 
                 * { 0 , 1 , 2 , 3 , 4} : len = 5: half_len: 2
                 *   2   1   0          : left  = half_len-1 to 0
                 *           0   1   2  : right = half_len+1 to len -1
                 * 
                 */
                while (current_sample->next != NULL) {
                    sum = 0;
                    testL = current_sample;
                    current_sample = current_sample->prev; // First on the left
                    for (int p = (half_len - 1); p >= 0; p--) {
                        sum += current_sample->hr * window[p];
                        current_sample = current_sample->prev;
                    }
                    current_sample = testL->next; // First on the right
                    for (int n = (half_len + 1); n < *len; n++) {
                        sum += current_sample->hr * window[n];
                        if (current_sample->next != NULL)
                            current_sample = current_sample->next;
                        else
                            return 0;
                    }
                    sum += testL->hr * window[half_len];
                    testL->hr = sum;

                    //verbose("%.2f\t%.2f\t%d\t%.2f Km/h\t%.2f m/Km\n", current_sample->hr, current_sample->distance, current_sample->time, current_sample->speed, current_sample->pace_min);
                    current_sample = testL->next;
                }

            } else {
                verbose("Invalid Window, Sum(window) != 1");
                return -1;
            }
        } else {
            verbose("Window Length shall be 2k-1\n");
            return -1;
        }
    } else {
        verbose("Head == %#010x or Window == %#010x\n", head, window);
        return -1;
    }
    return 0;
}

int average_pace_at_HR(Sample * head, const int * target_hr, int hr_tolerance, float * f_pace) {
    Sample * current_sample;
    int p_lines = 0;
    float sum = 0;
    if (head != NULL) {
        current_sample = head;
        while (current_sample->next != NULL) {
            
            if(abs(current_sample->hr - *target_hr) <= hr_tolerance){
                // verbose("%.2f\t%.2f\t%d\t%.2f Km/h\t%.2f m/Km\n", current_sample->hr, current_sample->distance, current_sample->time, current_sample->speed, current_sample->pace_min);
                sum += current_sample->pace_min;
                p_lines++;
            }
            
            current_sample = current_sample->next;
        }
        
        *f_pace = sum/p_lines;
        
    } else {
        verbose("Head == %#010x", head);
        return -1;
    }
    return p_lines;
}

int average_hr_at_Pace(Sample * head, const Pace * target_pace, float tolerance_factor, float * hr_at_pace){
    Sample * current_sample;
    Pace temp_min, temp_max;
    const char * min_str, * max_str;
    int h_lines = 0;
    float min, max, center;
    float sum = 0;
    
    center = pace_to_float(target_pace);
    min =  center - (center*tolerance_factor);
    max = center + (center*tolerance_factor);
    
    float_to_Pace(&temp_min, min);
    float_to_Pace(&temp_max, max);
    
    min_str = pace_to_str(&temp_min);
    max_str = pace_to_str(&temp_max);
            
    verbose("Searching for paces between [ %s , %s]\n", min_str , max_str);
    
    if (head != NULL) {
        current_sample = head;
        while (current_sample->next != NULL) {
            
            if(current_sample->pace_min < max && current_sample->pace_min > min){
                float_to_Pace(&current_sample->pace, current_sample->pace_min);
                // verbose("%s < %s < %s\n", min_str, pace_to_str(&current_sample->pace) , max_str );
                sum += current_sample->hr;
                h_lines++;
            }
            
            current_sample = current_sample->next;
        }
        
        *hr_at_pace = sum/h_lines;
        
    } else {
        verbose("Head == %#010x", head);
        return -1;
    }
    return h_lines;
}

