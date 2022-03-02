#include "driver.h"

driver_t* driver_create(size_t size){
  driver_t* driver = (driver_t*) malloc(sizeof(driver_t));
  unsigned int capacity = (unsigned int)size;
  driver->queue = queue_create(size);
  driver->open = 1;
  driver->s = 0;
  driver->h = 0;
  sem_init(&driver->empty, 0, capacity);
  sem_init(&driver->full, 0, 0);
  sem_init(&driver->mutex, 0, 1);
  sem_init(&driver->zq, 0, 1);
  sem_init(&driver->ready, 0, 1);
  return driver;
}







enum driver_status driver_schedule(driver_t *driver, void* job) {
  //////////////non zero queue/////////////////
  enum driver_status dstatus;
  enum queue_status qstatus;
  
  if (driver->queue->capacity > 0) {
    
  
    sem_wait(&driver->empty);
    sem_wait(&driver->mutex);
    ////////////////critical section start//////////////////
    if (driver->open == 1){
      qstatus = queue_add(driver->queue, job);
    
    }
    else {
      sem_post(&driver->mutex);
      sem_post(&driver->empty);
      sem_post(&driver->full);
      return DRIVER_CLOSED_ERROR;
    }
    ////////////////critical section end////////////////////
    sem_post(&driver->mutex);
    sem_post(&driver->full);

    if (qstatus == QUEUE_SUCCESS) {
      dstatus = SUCCESS;
    }
    else {
      dstatus = DRIVER_GEN_ERROR;
    }
    return dstatus;
    
  }
  
  //////////////zero queue/////////////////
  else {
    driver->s = driver->s + 1;
    sem_wait(&driver->zq);
    sem_wait(&driver->mutex);
    ////////////////critical section start//////////////////
    if (driver->open == 1){
      driver->job = job;
      qstatus = QUEUE_SUCCESS;
      dstatus = SUCCESS;
    }
    else {
      sem_post(&driver->mutex);
      sem_post(&driver->zq);
      sem_post(&driver->full);
      return DRIVER_CLOSED_ERROR;
    }
    //driver->s = driver->s - 1;
    ////////////////critical section end////////////////////
    sem_post(&driver->mutex);
    sem_post(&driver->full);
    
    
    return dstatus;
  }
}











enum driver_status driver_handle(driver_t *driver, void **job) {
  //////////////non zero queue/////////////////
  enum driver_status dstatus;
  enum queue_status qstatus;

  if (driver->queue->capacity > 0){
    
    sem_wait(&driver->full);
    sem_wait(&driver->mutex);
    ////////////////critical section start//////////////////
    if (driver->open == 1) {
      qstatus = queue_remove(driver->queue, job);
    }
    else {
      sem_post(&driver->mutex);
      sem_post(&driver->empty);
      sem_post(&driver->full);
      return DRIVER_CLOSED_ERROR;
    }
    ////////////////critical section end////////////////////
    sem_post(&driver->mutex);
    sem_post(&driver->empty);
  
    if (qstatus == QUEUE_SUCCESS) {
      dstatus = SUCCESS;
    }
    else {
      dstatus = DRIVER_GEN_ERROR;
    }
    return dstatus;
  }
  //////////////zero queue/////////////////
  else{
    driver->h = driver-> h + 1;
    sem_wait(&driver->full);
    sem_wait(&driver->mutex);
    ////////////////critical section start//////////////////
    if (driver->open == 1) {
      *job = driver->job;
      driver->job = NULL;
      qstatus = QUEUE_SUCCESS;
      dstatus = SUCCESS;
    }
    else {
      sem_post(&driver->mutex);
      sem_post(&driver->zq);
      sem_post(&driver->full);
      return DRIVER_CLOSED_ERROR;
    }
    driver->h = driver->h - 1;
    driver->s = driver->s - 1;
    ////////////////critical section end////////////////////
    sem_post(&driver->mutex);
    sem_post(&driver->zq);
    
    return dstatus;
  }
}









enum driver_status driver_non_blocking_schedule(driver_t *driver, void* job) {
  enum driver_status dstatus;
  enum queue_status qstatus;
  if (driver->h == 0 && driver->queue->capacity == 0){
    return DRIVER_REQUEST_FULL;
  }
  //////////////non zero queue/////////////////
  if (driver->queue->capacity > 0){
    
  
    if (sem_trywait(&driver->empty) == -1){
      return DRIVER_REQUEST_FULL;
    }
    if (sem_trywait(&driver->mutex)){
      return DRIVER_REQUEST_FULL;
    }
    
    ////////////////critical section start//////////////////
    if (driver->open == 1){
      qstatus = queue_add(driver->queue, job);
    
    }
    else {
      sem_post(&driver->mutex);
      sem_post(&driver->empty);
      sem_post(&driver->full);
      return DRIVER_CLOSED_ERROR;
    }
    ////////////////critical section end////////////////////
    sem_post(&driver->mutex);
    sem_post(&driver->full);
  
    if (qstatus == QUEUE_SUCCESS) {
      dstatus = SUCCESS;
    }
    else {
      dstatus = DRIVER_GEN_ERROR;
    }
    return dstatus;
  }
  //////////////zero queue/////////////////

  else {

    if (sem_trywait(&driver->zq) == -1){
      return DRIVER_REQUEST_FULL;
    }
    if (sem_trywait(&driver->mutex) == -1){
      return DRIVER_REQUEST_FULL;
    }
    ////////////////critical section start//////////////////
    if (driver->open == 1){
      driver->job = job;
      qstatus = QUEUE_SUCCESS;
      dstatus = SUCCESS;
    }
    else {
      sem_post(&driver->mutex);
      sem_post(&driver->zq);
      sem_post(&driver->full);
      return DRIVER_CLOSED_ERROR;
    }
    ////////////////critical section end////////////////////
    sem_post(&driver->mutex);
    sem_post(&driver->full);
    
    return dstatus;
  }
}











enum driver_status driver_non_blocking_handle(driver_t *driver, void **job) {
  enum driver_status dstatus;
  enum queue_status qstatus;
  if (driver->s == 0 && driver->queue->capacity == 0){
    return DRIVER_REQUEST_EMPTY;
  }
  //////////////non zero queue/////////////////
  if(driver->queue->capacity > 0){
    
  
    if (sem_trywait(&driver->full) == -1){
      return DRIVER_REQUEST_EMPTY;
    }
    if (sem_trywait(&driver->mutex) == -1){
      return DRIVER_REQUEST_EMPTY;
    }
    ////////////////critical section start//////////////////
    if (driver->open){
      qstatus = queue_remove(driver->queue, job);
    
    }
    else {
      sem_post(&driver->mutex);
      sem_post(&driver->empty);
      sem_post(&driver->full);
      return DRIVER_CLOSED_ERROR;
    }
    ////////////////critical section end////////////////////
    sem_post(&driver->mutex);
    sem_post(&driver->empty);
  
    if (qstatus == QUEUE_SUCCESS) {
      dstatus = SUCCESS;
    }
    else {
      dstatus = DRIVER_GEN_ERROR;
    }
    return dstatus;
    
  }
  //////////////zero queue/////////////////
  else{
    
    if (sem_trywait(&driver->full) == -1){
      return DRIVER_REQUEST_EMPTY;
    }
    if (sem_trywait(&driver->mutex) == -1){
      return DRIVER_REQUEST_EMPTY;
    }
    ////////////////critical section start//////////////////
    if (driver->open){
      *job = driver->job;
      driver->job = NULL;
      qstatus = QUEUE_SUCCESS;
      dstatus = SUCCESS;
    }
    else {
      sem_post(&driver->mutex);
      sem_post(&driver->empty);
      sem_post(&driver->full);
      return DRIVER_CLOSED_ERROR;
    }
    if (driver->s > 0){
      driver->s = driver->s - 1;
    }
    ////////////////critical section end////////////////////
    sem_post(&driver->mutex);
    sem_post(&driver->zq);
    
    return dstatus;
  }
}











enum driver_status driver_close(driver_t *driver) {
  if (driver == NULL) {
    return DRIVER_GEN_ERROR;
  }
  if (driver-> open == 0){
    return DRIVER_GEN_ERROR;
  }
  else {
    sem_wait(&driver->mutex);
    driver->open = 0;
    sem_post(&driver->mutex);
    sem_post(&driver->empty);
    sem_post(&driver->full);
  }
  
  return SUCCESS;
}

enum driver_status driver_destroy(driver_t *driver) {
  if (driver->open == 0){
    queue_free(driver->queue);
    sem_destroy(&driver->empty);
    sem_destroy(&driver->full);
    sem_destroy(&driver->mutex);
    free(driver);
    return SUCCESS;
  }
  else {
    return DRIVER_DESTROY_ERROR;
  }
}

enum driver_status driver_select(select_t *driver_list, size_t driver_count, size_t* selected_index) {
	/* IMPLEMENT THIS */
  enum driver_status dstatus;
  //enum queue_status qstatus;
  dstatus = SUCCESS;
  return dstatus;
}
