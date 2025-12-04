/*
  railway_reservation.c
  Console Railway Ticket Reservation (Intermediate C)
  Single-file program using file I/O to persist reservations.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define DATAFILE "tickets.dat"
#define MAX_SEATS 100
#define NAME_LEN 50
#define GENDER_LEN 10
#define PNR_LEN 32

typedef struct {
    char pnr[PNR_LEN];
    char name[NAME_LEN];
    int age;
    char gender[GENDER_LEN];
    int seat_no;         // 1..MAX_SEATS
    int active;          // 1 = booked, 0 = cancelled/freed
} Ticket;

void clear_stdin(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {}
}

void press_enter_to_continue() {
    printf("\nPress Enter to continue...");
    getchar();
}

// Generate a simple PNR using timestamp + random number
void generate_pnr(char *out, size_t len) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    int r = rand() % 10000;
    snprintf(out, len, "PNR%02d%02d%02d%02d%04d",
             tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, r);
}

// Load all tickets into an array and return count
int load_all_tickets(Ticket tickets[], int max) {
    FILE *f = fopen(DATAFILE, "rb");
    if (!f) return 0;
    int i = 0;
    while (i < max && fread(&tickets[i], sizeof(Ticket), 1, f) == 1) {
        i++;
    }
    fclose(f);
    return i;
}

// Save all tickets from array (count) to file (overwrite)
int save_all_tickets(Ticket tickets[], int count) {
    FILE *f = fopen(DATAFILE, "wb");
    if (!f) {
        perror("Unable to open data file for writing");
        return 0;
    }
    fwrite(tickets, sizeof(Ticket), count, f);
    fclose(f);
    return 1;
}

// Append a ticket to the data file
int append_ticket_to_file(const Ticket *t) {
    FILE *f = fopen(DATAFILE, "ab");
    if (!f) {
        perror("Unable to open data file for appending");
        return 0;
    }
    fwrite(t, sizeof(Ticket), 1, f);
    fclose(f);
    return 1;
}

// Mark ticket with given pnr as cancelled (active=0). Returns 1 if cancelled.
int cancel_ticket_by_pnr(const char *pnr) {
    FILE *f = fopen(DATAFILE, "r+b");
    if (!f) {
        printf("No bookings found.\n");
        return 0;
    }
    Ticket t;
    int found = 0;
    while (fread(&t, sizeof(Ticket), 1, f) == 1) {
        if (t.active && strcmp(t.pnr, pnr) == 0) {
            t.active = 0;
            fseek(f, - (long)sizeof(Ticket), SEEK_CUR);
            fwrite(&t, sizeof(Ticket), 1, f);
            found = 1;
            break;
        }
    }
    fclose(f);
    return found;
}

int seat_is_taken(int seat_no) {
    FILE *f = fopen(DATAFILE, "rb");
    if (!f) return 0;
    Ticket t;
    int taken = 0;
    while (fread(&t, sizeof(Ticket), 1, f) == 1) {
        if (t.active && t.seat_no == seat_no) {
            taken = 1;
            break;
        }
    }
    fclose(f);
    return taken;
}

// Get next available seat number (1..MAX_SEATS), or -1 if full
int get_next_available_seat() {
    for (int s = 1; s <= MAX_SEATS; ++s) {
        if (!seat_is_taken(s)) return s;
    }
    return -1;
}

// Count active bookings
int count_active_bookings() {
    FILE *f = fopen(DATAFILE, "rb");
    if (!f) return 0;
    Ticket t;
    int c = 0;
    while (fread(&t, sizeof(Ticket), 1, f) == 1) {
        if (t.active) c++;
    }
    fclose(f);
    return c;
}

void book_ticket_console() {
    Ticket t;
    memset(&t, 0, sizeof(Ticket));
    char temp[128];

    printf("\n--- Book Ticket ---\n");
    printf("Passenger name: ");
    fgets(temp, sizeof(temp), stdin);
    if (temp[strlen(temp)-1] == '\n') temp[strlen(temp)-1] = '\0';
    strncpy(t.name, temp, NAME_LEN-1);

    printf("Age: ");
    if (scanf("%d", &t.age) != 1) {
        printf("Invalid age input.\n");
        clear_stdin();
        return;
    }
    clear_stdin();

    printf("Gender (M/F/O): ");
    fgets(temp, sizeof(temp), stdin);
    if (temp[strlen(temp)-1] == '\n') temp[strlen(temp)-1] = '\0';
    strncpy(t.gender, temp, GENDER_LEN-1);

    int seat = get_next_available_seat();
    if (seat == -1) {
        printf("Sorry, no seats available.\n");
        return;
    }
    t.seat_no = seat;
    t.active = 1;
    generate_pnr(t.pnr, sizeof(t.pnr));
    if (!append_ticket_to_file(&t)) {
        printf("Failed to save booking.\n");
        return;
    }

    printf("\nBooking successful!\n");
    printf("PNR: %s\nName: %s\nAge: %d\nGender: %s\nSeat No: %d\n",
           t.pnr, t.name, t.age, t.gender, t.seat_no);
}

void cancel_ticket_console() {
    char pnr[PNR_LEN];
    printf("\n--- Cancel Ticket ---\n");
    printf("Enter PNR: ");
    fgets(pnr, sizeof(pnr), stdin);
    if (pnr[strlen(pnr)-1] == '\n') pnr[strlen(pnr)-1] = '\0';

    if (cancel_ticket_by_pnr(pnr)) {
        printf("Ticket %s cancelled successfully.\n", pnr);
    } else {
        printf("PNR not found or already cancelled.\n");
    }
}

void view_all_bookings_console() {
    FILE *f = fopen(DATAFILE, "rb");
    if (!f) {
        printf("\nNo bookings found.\n");
        return;
    }
    Ticket t;
    int any = 0;
    printf("\n--- All Active Bookings ---\n");
    printf("%-12s %-20s %-4s %-6s %-6s\n", "PNR", "Name", "Age", "Gender", "Seat");
    printf("----------------------------------------------------------------\n");
    while (fread(&t, sizeof(Ticket), 1, f) == 1) {
        if (t.active) {
            printf("%-12s %-20s %-4d %-6s %-6d\n", t.pnr, t.name, t.age, t.gender, t.seat_no);
            any = 1;
        }
    }
    if (!any) printf("No active bookings.\n");
    fclose(f);
}

void search_by_pnr_console() {
    char pnr[PNR_LEN];
    printf("\n--- Search Booking by PNR ---\n");
    printf("Enter PNR: ");
    fgets(pnr, sizeof(pnr), stdin);
    if (pnr[strlen(pnr)-1] == '\n') pnr[strlen(pnr)-1] = '\0';

    FILE *f = fopen(DATAFILE, "rb");
    if (!f) {
        printf("No bookings found.\n");
        return;
    }
    Ticket t;
    int found = 0;
    while (fread(&t, sizeof(Ticket), 1, f) == 1) {
        if (strcmp(t.pnr, pnr) == 0) {
            found = 1;
            if (t.active) {
                printf("\nPNR: %s\nName: %s\nAge: %d\nGender: %s\nSeat: %d\n",
                       t.pnr, t.name, t.age, t.gender, t.seat_no);
            } else {
                printf("PNR %s was cancelled earlier.\n", pnr);
            }
            break;
        }
    }
    if (!found) printf("PNR not found.\n");
    fclose(f);
}

void show_available_seats_console() {
    printf("\n--- Seat Map (X = booked, O = available) ---\n");
    int per_row = 10;
    for (int s = 1; s <= MAX_SEATS; ++s) {
        int taken = seat_is_taken(s);
        printf("%3d[%c] ", s, taken ? 'X' : 'O');
        if (s % per_row == 0) printf("\n");
    }
    printf("\nTotal seats: %d | Booked: %d | Available: %d\n",
           MAX_SEATS, count_active_bookings(), MAX_SEATS - count_active_bookings());
}

void menu() {
    int choice;
    srand((unsigned)time(NULL));
    while (1) {
        printf("\n====== Railway Reservation System ======\n");
        printf("1. Book Ticket\n");
        printf("2. Cancel Ticket\n");
        printf("3. View All Bookings\n");
        printf("4. Search by PNR\n");
        printf("5. Show Available Seats\n");
        printf("0. Exit\n");
        printf("Enter choice: ");
        if (scanf("%d", &choice) != 1) {
            clear_stdin();
            printf("Invalid input. Try again.\n");
            continue;
        }
        clear_stdin();
        switch (choice) {
            case 1: book_ticket_console(); press_enter_to_continue(); break;
            case 2: cancel_ticket_console(); press_enter_to_continue(); break;
            case 3: view_all_bookings_console(); press_enter_to_continue(); break;
            case 4: search_by_pnr_console(); press_enter_to_continue(); break;
            case 5: show_available_seats_console(); press_enter_to_continue(); break;
            case 0: printf("Goodbye!\n"); return;
            default: printf("Invalid choice.\n"); press_enter_to_continue(); break;
        }
    }
}

int main(void) {
    menu();
    return 0;
}