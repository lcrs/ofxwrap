void setups_save(char *path, char *file) {
    char *f = (char *) malloc(strlen(path) + strlen("/") + strlen(file) + strlen("_ofxsetup"));
    strcpy(f, path);
    strcat(f, "/");
    strcat(f, file);
    strcat(f, "_ofxsetup");
    printf("Ofxwrap: in setups_save(), going to %s\n", f);
    free(f);
}

void setups_load(char *path, char *file) {

}
