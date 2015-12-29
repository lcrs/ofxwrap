char * setups_getpath(char *path, char *file) {
  char *f = (char *) malloc(500);
  strcpy(f, path);
  strcat(f, "/");
  strcat(f, file);
  strcat(f, "_ofxsetup");
  return f;
}

void setups_save(char *path, char *file) {
  char *p = setups_getpath(path, file);
  printf("Ofxwrap: in setups_save(), going to %s\n", p);
  FILE *f = fopen(p, "w");
  if(f == NULL) {
    printf("Ofxwrap: in setups_save(), failed to open %s for writing!\n", p);
  } else {
    char *s = (char *) malloc(200*1024);
    sprintf(s, "DNP=%s\nNFP=%s\nParamsHash1=%d\nParamsHash2=%d\nParamsHash3=%d\n",
                  dnp_data, nfp_data, paramshash1_data, paramshash2_data, paramshash3_data);
    fwrite(s, strlen(s), 1, f);
    free(s);
  }
  fclose(f);
  free(p);
}

void setups_load(char *path, char *file) {
  char *p = setups_getpath(path, file);
  printf("Ofxwrap: in setups_load(), coming from %s\n", p);
  FILE *f = fopen(p, "r");
  if(f == NULL) {
    printf("Ofxwrap: in setups_load(), failed to open %s for reading!\n", p);
  } else {
    dnp_data = (char *) realloc(dnp_data, 100*1024);
    nfp_data = (char *) realloc(nfp_data, 100*1024);
    fscanf(f, "DNP=%s\nNFP=%s\nParamsHash1=%d\nParamsHash2=%d\nParamsHash3=%d\n",
               dnp_data, nfp_data, &paramshash1_data, &paramshash2_data, &paramshash3_data);
    printf("Ofxwrap: in setups_load(), DNP %.10s, NFP %.10s, hashes %d %d %d\n", dnp_data, nfp_data, paramshash1_data, paramshash2_data, paramshash3_data);
    if((strcmp(dnp_data, "(null)") == 0) || (strcmp(nfp_data, "(null)") == 0)) {
      free(dnp_data); dnp_data = NULL;
      free(nfp_data); nfp_data = NULL;
    }
  }
  fclose(f);
  free(p);
}
