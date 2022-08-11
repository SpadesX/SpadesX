# Contributors Guide
If you are interested in making this project better, there are many ways you can contribute. For example, you can:

- Submit a bug report.
- Suggest a new feature.
- Provide feedback by commenting on feature requests/proposals.
- Propose a patch by submitting a pull request.
- Suggest or submit documentation improvements.
- Review outstanding pull requests.
- Answer questions from other users.
- Share the software with other users who are interested.
- Teach others to use the software.
- Package and distribute the software in a downstream community (such as your preferred Linux distribution).

## Bugs and Feature Requests
If you believe that you have found a bug or wish to propose a new feature, please first search the existing [issues] to see if it has already been reported.
If you are unable to find an existing issue, consider using one of the provided templates to create a new issue and provide as many details as you can to assist in reproducing the bug or explaining your proposed feature.
If you believe that you have found a security bug it would be preferred to open a ticket in the "spadesx-bugs" channel in the [official Discord][discord].

## Patch Submission tips
Patches should be submitted in the form of Pull Requests to the [repository] on GitHub.
 But first, consider the following tips to ensure a smooth process when submitting a patch:

- Ensure that the patch compiles and does not break any build-time tests.
- Be understanding, patient, and friendly; developers may need time to review
  your submissions before they can take action or respond. This does not mean
  your contribution is not valued. If your contribution has not received a
  response in a reasonable time, consider commenting with a polite inquiry for
  an update.
- Limit your patches to the smallest reasonable change to achieve your intended
  goal. For example, do not make unnecessary indentation changes; but don't go
  out of your way to make the patch so minimal that it isn't easy to read,
  either. Consider the reviewer's perspective.
- Avoid "find and replace" changes in your pull request, unless justified.  While it may be 
  tempting to globally replace calls to deprecated methods or change the style
  of the code to fit your personal preference, these types of seemingly trivial
  changes have likely not already been performed by the SpadesX team for good 
  reason.
- Focus your patches on bug fixes that were discovered through real-world
  usage and testing, and on improvements that clearly satisfy a need in 
  SpadesX's functionality.  Before you begin implementing, consider first
  opening a dialogue with the SpadesX team to ensure that your efforts will 
  align with the goals of the project or look at the [issues]. This will significantly improve the odds
  that your patch gets accepted.
- Unless it addresses a critical security update, avoid pull requests that update 
  other 3rd party libraries.  It is preferred that these changes are made
  internally by the team.  If you have a need for an updated library, please
  submit an issue with your pull request instead of a lone pull request.
- Isolate multiple patches from each other. If you wish to make several
  independent patches, do so in separate, smaller pull requests that can be
  reviewed more easily.
- Be prepared to answer questions from reviewers. They may have further
  questions before accepting your patch, and may even propose changes. Please
  accept this feedback constructively, and not as a rejection of your proposed
  change.

## Review
- We welcome code reviews from anyone. A committer is required to formally
  accept and merge the changes.
- Reviewers will be looking for things like threading issues, performance
  implications, API design, duplication of existing functionality, readability
  and code style, avoidance of bloat (scope-creep), etc.
- Reviewers will likely ask questions to better understand your change.
- Reviewers will make comments about changes to your patch:
    - MUST means that the change is required
    - SHOULD means that the change is suggested, further discussion on the
      subject may be required
    - COULD means that the change is optional

## Timeline and Managing Expectations
As we continue to engage contributors and learn best practices for running a successful open source project, our processes and guidance will likely evolve.
We will try to communicate expectations as we are able and to always be responsive. We hope that the community will share their suggestions for improving this engagement.
Based on the level of initial interest we receive and the availability of resources to evaluate contributions, we anticipate the following:

- We will initially prioritize pull requests that include small bug fixes and code that addresses potential vulnerabilities
  as well as pull requests that include improvements for optimizations because these require a reasonable amount of effort 
  to evaluate and will help us exercise and revise our process for accepting contributions. In other words, we are going to start small in order to work out the kinks first.
- We are committed to maintaining the integrity and security of our code base.  In addition to the careful review the 
  maintainers will give to code contributions to make sure they do not introduce new bugs or vulnerabilities, we will be 
  trying to identify best practices to incorporate with our open source project so that contributors can have more control 
  over whether their contributions are accepted. These might include things like style guides and requirements for tests and 
  documentation to accompany some code contributions.  As a result, it may take a long time for some contributions to be 
  accepted.  This does not mean we are ignoring them.
- Not all innovative ideas need to be accepted as pull requests into this GitHub project to be valuable to the community.
  There may be times when we recommend that you just share your code for some enhancement from your own repository.
  As we identify and recognize extensions/scripts that are of general interest to the Ace of Spades community, we may seek to incorporate them with our baseline.


[issues]: https://github.com/SpadesX/SpadesX/issues
[repository]: https://github.com/SpadesX/SpadesX/
[discord]: https://discord.gg/dsRjTzJpZC
