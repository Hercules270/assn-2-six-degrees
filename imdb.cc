using namespace std;
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include "imdb.h"
#include <cstring>
#include <string>

const char *const imdb::kActorFileName = "actordata";
const char *const imdb::kMovieFileName = "moviedata";

typedef struct
{
  const void *key;
  const void *file;
} info;

imdb::imdb(const string &directory)
{
  const string actorFileName = directory + "/" + kActorFileName;
  const string movieFileName = directory + "/" + kMovieFileName;

  actorFile = acquireFileMap(actorFileName, actorInfo);
  movieFile = acquireFileMap(movieFileName, movieInfo);
}

bool imdb::good() const
{
  return !((actorInfo.fd == -1) ||
           (movieInfo.fd == -1));
}

//This method is called by bsearch for comparing two actors names
int cmpPlayers(const void *a, const void *b)
{
  info information = *(info *)a;
  char *firstPlayer = (char *)information.key;
  const void *actorFile = information.file;
  int offset = *(int *)b;
  char *secondPlayer = (char *)actorFile + offset;

  return strcmp(firstPlayer, secondPlayer);
}

//This method creates film struct based on movieFIle and offset where movie is
film makeFilm(int offset, const void *movieFile)
{
  char *pointer = (char *)movieFile + offset;
  string title(pointer);
  pointer = pointer + title.size() + 1;
  int year = 1900 + *pointer;
  film secondFilm;
  secondFilm.title = title;
  secondFilm.year = year;
  return secondFilm;
}

//This method fills vector of films with movies of the player got as argument
void fillVectorWithFilms(void *player, const void *movieFile, vector<film> &films)
{
  int offset = strlen((char *)player);
  offset += (2 - (offset % 2));
  player = (char *)player + offset;
  short numberOfMovies = *(short *)player;
  offset += 2 + 2 - (offset % 4) - offset;
  player = (char *)player + +4 - (offset % 4);
  for (int i = 0; i < numberOfMovies; i++)
  {
    string title((char *)movieFile + *(int *)player);
    film curr = makeFilm(*(int *)player, movieFile);
    films.push_back(curr);
    player = (char *)player + sizeof(int);
  }
}

// you should be implementing these two methods right here...
bool imdb::getCredits(const string &player, vector<film> &films) const
{
  int numberOfPlayers = *(int *)actorFile;      //Gent number of players in array
  void *base = (char *)actorFile + sizeof(int); //Get base for bsearch function
  char *playerName = strdup(player.c_str());

  //Create struct with actor name and actorFile to pass a key value to bsearch
  info information;
  information.key = playerName;
  information.file = actorFile;

  void *searchResult = bsearch(&information, base, numberOfPlayers, sizeof(int), cmpPlayers);
  if (searchResult == NULL) //I nothing was found return false
    return false;
  free(playerName);
  searchResult = (char *)actorFile + *(int *)searchResult; //get the pointer to the actor
  fillVectorWithFilms(searchResult, movieFile, films);     //Fill the vector with films
  return true;
}

//This method is called bsearch to compare two movies
int cmpMovies(const void *a, const void *b)
{
  info information = *(info *)a;
  const film firstFilm = *(const film *)information.key;
  const void *movieFile = information.file;
  int offset = *(int *)b;
  film secondFilm = makeFilm(offset, movieFile);
  if (firstFilm < secondFilm)
    return -1;
  else if (firstFilm == secondFilm)
    return 0;
  return 1;
}

//This method fills vector with actors
void fillVectorWithPlayers(void *movie, const void *actorFile, vector<string> &players)
{
  int titleLength = strlen((char *)movie);
  int offset = titleLength + 2 + (titleLength % 2);
  movie = (char *)movie + offset;
  int numberOfPlayers = (int)*(short *)movie;
  movie = (char *)movie + 4 - (offset % 4);

  for (int i = 0; i < numberOfPlayers; i++)
  {
    string tmp((char *)actorFile + *(int *)movie);
    players.push_back(tmp);
    movie = (char *)movie + sizeof(int);
  }
}

bool imdb::getCast(const film &movie, vector<string> &players) const
{
  int numberOfMovies = *(int *)movieFile; //Get the number of movies in array
  void *base = (char *)movieFile + 4;     //Get base of array for bsearch
  //Create struct with movie struct and movieFile in it which will be passed as key to bsearh
  info information;
  information.key = &movie;
  information.file = movieFile;
  void *searchResult = bsearch(&information, base, numberOfMovies, sizeof(int), cmpMovies);
  if (searchResult == NULL) //If nothing was found return false
    return false;

  searchResult = (char *)movieFile + *(int *)searchResult; //Get the pointer to the movie
  fillVectorWithPlayers(searchResult, actorFile, players); //Fill the vector with actor names

  return true;
}

imdb::~imdb()
{
  releaseFileMap(actorInfo);
  releaseFileMap(movieInfo);
}

// ignore everything below... it's all UNIXy stuff in place to make a file look like
// an array of bytes in RAM..
const void *imdb::acquireFileMap(const string &fileName, struct fileInfo &info)
{
  struct stat stats;
  stat(fileName.c_str(), &stats);
  info.fileSize = stats.st_size;
  info.fd = open(fileName.c_str(), O_RDONLY);
  return info.fileMap = mmap(0, info.fileSize, PROT_READ, MAP_SHARED, info.fd, 0);
}

void imdb::releaseFileMap(struct fileInfo &info)
{
  if (info.fileMap != NULL)
    munmap((char *)info.fileMap, info.fileSize);
  if (info.fd != -1)
    close(info.fd);
}
