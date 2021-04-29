# Group 8

## Requirements
* Ubuntu 18.X
* Docker 20.10.X
* Docker-compose 1.28.X
* g++ 9.3.X
* Git

## Set-up
### Docker Build & Run
1. In a terminal, change to the directory with the downloaded recording files
2. In the same terminal, run opendlv-vehicle-view
   1. `docker run --rm --init --net=host --name=opendlv-vehicle-view -v $PWD:/opt/vehicle-view/recordings -v /var/run/docker.sock:/var/run/docker.sock -p 8081:8081 chalmersrevere/opendlv-vehicle-view-multi:v0.0.60`
3. Open opendlv-vehicle-view in Chrome & select a recording to play
   1. `http://localhost:8081`
4. In a new terminal, enable access to the GUI
   1. `xhost +`
5. In the same terminal, run the h264decoder
   1. `docker run --rm -ti --net=host --ipc=host -e DISPLAY=$DISPLAY -v /tmp:/tmp h264decoder:v0.0.4 --cid=253 --name=img`
6. Play the recording (if you haven't already) to add data to the shared memory
7. In a new terminal, clone repository into a directory of your choosing using SSH
   1. `git clone git@git.chalmers.se:courses/dit638/students/2021-group-08.git`
8. In the same terminal, change directory to the repository
   1. `cd 2021-group-08`
9. In the same terminal, build the repository
   1. `docker build -f Dockerfile -t driveryourself:latest .`
10. In the same terminal, run DriverYourself & play the recording in Chrome
   1. `docker run --rm -ti --net=host --ipc=host -e DISPLAY=$DISPLAY -v /tmp:/tmp driveryourself:latest --cid=253 --name=img --width=640 --height=480 --verbose`

### Manual Build
1. Clone repository into a directory of your choosing using SSH
`git clone git@git.chalmers.se:courses/dit638/students/2021-group-08.git`
2. Change directory to the repository
   1. `cd 2021-group-08`
3. Create build directory
   1. `mkdir build`
4. Change to build repository
   1. `cd build`
5. Build software
   1. `cmake ..`
   2. `make`
6. To run the test suites 
   1. `make test`
 
## Workflow
### Add new features
1. Add Trello card and assign team members
2. Create issue in gitlab
   1. Add requirement the issue is covering
3. Create new branch for new feature
   1. Only functioning code in master branch
   2. Code Review: Team member with least possible involvement reviews committed code for new feature
4. Write unit test for new feature before creating merge request
   1. Those responsible for the feature will write the unit tests
5. Update corresponding Trello card with new status
6. Features will be merged into a branch when they are completed & CI passes
   1. At least one member approves of the completed feature
   2. Rebasing from master will be used when needed

### Bugfixes
1. Add Trello card and assign team members
2. Create issue in gitlab
3. If bug is discovered after feature has been committed to master: create new branch for bug fixing
4. If bug is discovered while working in a branch for a new feature: continue in existing branch and fix the bug before committing branch to master
5. Add missing unit test for feature that had a bug
6. Update corresponding Trello card with new status
7. Team member with least possible involvement reviews committed code for bug fix

### Commit messages
Adapted from: https://chris.beams.io/posts/git-commit/

#### Style
* Short subject line
* Capitalize beginning of subject line
* No period at the end of subject line
* Imperative mood in the subject line
* Use tags in the beginning of the subject line
   * [FIX]
   * [IMPLEMENT]
   * [INIT]
   * [TEST]

#### Content
* What, why and how
   * What: feature, bug, method names
   * Why: bugfix, explains why you solved it this way
   * How: briefly how it was implemented
* At least 1 sentence or 1 mention

#### Metadata
* Co-author tag used when pair programming or collaborating
   * Add to top of message, under the subject line (see example)
* Issue tracking ID (“Resolves issue #$”)
* We plan to merge into a branch rather than rebasing.

##### Example
```
[IMPLEMENT] Summarize changes in around 50 characters or less

Co-authored-by: chalmersid <chalmersid@student.chalmers.se>

More detailed explanatory text, if necessary. Wrap it to about 72
characters or so. In some contexts, the first line is treated as the
subject of the commit and the rest of the text as the body. The
blank line separating the summary from the body is critical (unless
you omit the body entirely); various tools like `log`, `shortlog`
and `rebase` can get confused if you run the two together.

Explain the problem that this commit is solving. Focus on why you
are making this change as opposed to how (the code explains that).
Are there side effects or other unintuitive consequences of this
change? Here's the place to explain them.

Resolves: #123
```
