#include "pizza.h"


uint rng = 0;   //input number that will be used for generate random numbers

// variables for mutexes
volatile int tel_being_used = 0;
volatile int cooks_being_used = 0;
volatile int ovens_being_used = 0;
volatile int delivers_being_used = 0;
volatile int packer = 0;
// mutex to be used for various things to be done
pthread_mutex_t tel_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t cook_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t oven_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t deliver_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t pack_lock = PTHREAD_MUTEX_INITIALIZER;

// mutex to be used for main function thread
pthread_mutex_t main_lock = PTHREAD_MUTEX_INITIALIZER;

// conditions for while in mutex
pthread_cond_t tel_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t cook_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t oven_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t deliver_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t pack_cond = PTHREAD_COND_INITIALIZER;


// variables for queues 
volatile int tel_queue = 0;
volatile int cook_queue = 0;
volatile int oven_queue = 0;
volatile int deliver_queue = 0;
volatile int pack_queue = 0;


//variables for output
volatile int total_earnings = 0;
volatile int total_orders = 0;
volatile int missed_orders = 0;


void deliver(){
    int time_to_deliver;
    int i;
    pthread_mutex_lock(&deliver_lock);
    
    while(1){
        
        delivers_being_used++;
        if(delivers_being_used <= Ndeliver){
            
            time_to_deliver = (rand_r(&rng) % Tdelhigh) + Tdellow;
            //printf("time needed for delivery: %d\n", 2 * time_to_deliver);
            sleep(2 * time_to_deliver);
            //printf("here\n");
            break;
        }
        else{
            delivers_being_used--;
            deliver_queue++;
            printf("All delivers are busy.\n");
            pthread_cond_wait(&deliver_cond, &deliver_lock);
        }
    }

    delivers_being_used--;
    for(i = 0; i < deliver_queue; i++){
        pthread_cond_signal(&deliver_cond);
    }
    deliver_queue = 0;
    pthread_mutex_unlock(&deliver_lock);
}

void pack(int pizzas){
    int i;
    pthread_mutex_lock(&pack_lock);
    while(1){
        
        if(packer == 0){
            packer = 1;
            
            sleep(pizzas * Tpack);
            break;
        }   
        else{
            printf("waiting in pack\n");
            pack_queue++;
            pthread_cond_wait(&pack_cond, &pack_lock);
        }
    }
    packer--;
    for(i = 0; i < pack_queue; i++){
        pthread_cond_signal(&pack_cond);
    }
    pack_queue = 0;
    pthread_mutex_unlock(&pack_lock);
}

void oven(int pizzas){
    int i;
    pthread_mutex_lock(&oven_lock);
    while(1){
        
        ovens_being_used += pizzas;
        
        if(ovens_being_used <= Noven){
            cooks_being_used--;
            //pthread_mutex_unlock(&oven_lock);
            pthread_cond_signal(&cook_cond);
            sleep(Tbake);
            break;
        }
        else{
            ovens_being_used -= pizzas;
            oven_queue++;
            printf("Not enough ovens for this order at the moment.\n");
            
            pthread_cond_wait(&oven_cond, &oven_lock);
        }
    }
    //pthread_mutex_lock(&oven_lock);
    ovens_being_used -= pizzas; 
    for(i = 0; i < oven_queue; i++){
        pthread_cond_signal(&oven_cond);
    }
    oven_queue = 0;
    pthread_mutex_unlock(&oven_lock);
}

void cook(int pizzas){
    int i;
    pthread_mutex_lock(&cook_lock);
    while(1){
        
        cooks_being_used++;
        
        
        if(cooks_being_used <= Ncook){
            //pthread_mutex_unlock(&cook_lock);
            sleep(Tprep);
            oven(pizzas);
            //printf("cooks: %d\n", cooks_being_used);
            break;
        }
        else {
            cooks_being_used--;
            printf("All cooks are being used at the moment.\n");
            cook_queue++;
            pthread_cond_wait(&cook_cond, &cook_lock);
        }
    }
    //pthread_mutex_lock(&cook_lock);
    cooks_being_used--;
    for(i = 0; i < cook_queue; i++){
        pthread_cond_signal(&cook_cond);
    }
    cook_queue = 0;
    pthread_mutex_unlock(&cook_lock);
}

int call(){
    int get_sleep;
    int get_pizza;
    int get_time;
    int fail;
    int i;
    pthread_mutex_lock(&tel_lock);
    if(total_orders == 0){
        
        total_orders++;
    }
    else{
        get_sleep = (rand_r(&rng) % Torderhigh) + Torderlow;    //wait rnd time between [ToH,ToL]
        sleep(get_sleep);
        total_orders++;
    }
    while(1){
        tel_being_used++;
        if(tel_being_used <= Ntel){
            get_pizza = (rand_r(&rng) % Norderhigh) + Norderlow;    // get rnd pizza between [NH,NL]
            get_time = (rand_r(&rng) % Tpaymenthigh) + Tpaymentlow; // get rnd time between [TpH,TpL]
            sleep(get_time);    // wait for get_time to establish the order
            fail = rand_r(&rng) % 100 + 1;
            fail = fail / 100;
            if(fail == Pfail){
                pthread_cond_signal(&tel_cond);
                pthread_mutex_unlock(&tel_lock);
                return -1;
            }
            break;
        }
        else{
            printf("All telephones are being used at the moment.\n");
            tel_queue++;
            pthread_cond_wait(&tel_cond, &tel_lock);
        }
    }
    //pthread_mutex_lock(&tel_lock);
    tel_being_used--;
    for(i = 0; i < tel_queue; i++){
        pthread_cond_signal(&tel_cond);
    }
    tel_queue = 0;
    pthread_mutex_unlock(&tel_lock);

    return get_pizza;
}


void *pizza_restaurant(void *pid){
    
    int id = (int) pid;
    struct timespec start, finish, pre_deliver;
    int pizzas_order = 3;        // variable to store the order

    //function for making the order 
    pizzas_order = call();
    clock_gettime(CLOCK_REALTIME, &start);
    pthread_mutex_lock(&main_lock);
    if(pizzas_order == -1){
        // order has been declined
        printf("Η παραγγελία με αριθμό <%d> απέτυχε. %f\n", id, ((double) start.tv_sec) /Nsec );
        missed_orders++;
        return NULL;
    }
    // order has been accepted
    printf("Η παραγγελία με αριθμό <%d> καταχωρήθηκε με %d πίτσες. \n", id, pizzas_order/* , ((double) start.tv_sec) */  );
    total_earnings += pizzas_order * Cpizza;   // add money to totall earnings
    pthread_mutex_unlock(&main_lock);

    // function that includes making the pizza and baking it
    cook(pizzas_order);
    //printf("Η παραγγελία με αριθμό <%d> μαγειρευτηκε.\n", id);
    pack(pizzas_order);

    clock_gettime(CLOCK_REALTIME, &pre_deliver);
    pthread_mutex_lock(&main_lock);
    printf("Η παραγγελία με αριθμό <%d> ετοιμάστηκε σε %f λεπτά.\n", id, ((double) (pre_deliver.tv_sec - start.tv_sec)) );
    pthread_mutex_unlock(&main_lock);

    //function for deliver
    deliver();

    clock_gettime(CLOCK_REALTIME, &finish);
    pthread_mutex_lock(&main_lock);
    printf("Η παραγγελία με αριθμό <%d> παραδόθηκε σε %f λεπτά.\n", id, ((double) (finish.tv_sec - start.tv_sec)) );
    pthread_mutex_unlock(&main_lock);

    return NULL;
}


int main(int argc, char *argv[]){

    // managing input 
    if(argc != 3){
        printf("Wrong number of parameters !\nYou need to provide one parameter for # of customers and one # for generating random numbers!\nExiting ....\n");
        exit(EXIT_FAILURE);
    }

    int Ncust;
    Ncust = atoi(argv[1]);
    rng = atoi(argv[2]);
    printf("Customers: %d, rng: %d\n", Ncust, rng);

    // all input has been saved 

    // creating threads
    pthread_t threads[Ncust];
    int i;


    for(i = 0; i < Ncust; i++){
        if(pthread_create(&threads[i], NULL, pizza_restaurant, i+1) < 0){
            fprintf(stderr, "Error in semctl: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    // threads have been created and running

    //wait for threads to finish
    for(i = 0; i < Ncust; i++){
        pthread_join(threads[i], NULL);
    }

    printf("Total earnings: %d.\n", total_earnings);
    printf("Total orders accepted: %d. \n", total_orders - missed_orders);
    printf("");

    return 0;
}