#ifndef UI_H
#define UI_H

char *get_input_directory();
int get_thread_count();
char *get_edit_type();

void display_processing_result(const char *edit_type, int count, double elapsed);
void display_final_statistics(int total_processed, double total_time, int num_threads);

#endif