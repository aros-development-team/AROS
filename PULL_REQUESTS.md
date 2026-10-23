# Pull Request Process

This document describes the process for submitting pull requests to AROS. Please also read
[CONTRIBUTING.md](CONTRIBUTING.md) for general guidance on contributing to the project.

## Scope of a pull request

Please keep each pull request focused on a single logical change where possible, and avoid
including unrelated formatting, refactoring, or other changes in the same pull request.

A pull request should also be limited to a single component or subsystem. Do not submit pull
requests that touch unrelated parts of the repository. If a change genuinely requires modifications
to more than one component, explain why in the pull request description.

Pull requests must stand on their own merits. Do not open a pull request that depends on another
pull request which has not yet been reviewed and accepted. If a change builds on work that is
still under review, wait until that work has been merged before submitting the dependent change.
If a set of changes cannot be separated, submit them as a single pull request.

## Licensing and copyright

AROS is licensed under the [AROS Public License (APL)](LICENSE). By submitting a pull request you
agree that your contribution is made available under the APL.

Code and components written for AROS are contributed to the project as a whole, and must carry
the standard AROS copyright notice:

    Copyright (C) 1995-<year>, The AROS Development Team. All rights reserved.

Do not assign copyright of code or components you contribute to AROS to yourself, and do not add
personal copyright notices or alternative licence terms to files written for AROS. Pull requests
that do so will not be accepted.

Code derived from an external source must retain its original copyright notice and licence. Such
code is used under the terms of its own licence, and must be handled in accordance with that licence
and the intentions of its original authors, including any requirements for attribution, notice
preservation, or distribution of source. Clearly identify any such code, its origin, and its licence
in the pull request description.

If you wish to grant additional permissions for your own changesets to be used in other projects,
you may add yourself to [LICENSE.Author](LICENSE.Author). This does not change the licensing of
AROS itself.

## Checklist

1. Ensure any code that is not derived from an external source follows the [AROS coding conventions](http://developers.aros.org/documentation/styleguide.html).
   C and header source files should use Unix (LF) line endings, consistent with those conventions
   and the repository's `.astyle` configuration.
   Take special care when tidying MUI UI source layouts: the `.astyle` configuration can mangle
   these layouts, so verify that they have not been malformed.

2. Update the AROS copyright notice year in any modified source files where a copyright notice is
   present. Do not modify copyright notices in files derived from external sources unless required
   by their applicable licensing terms.

3. Make sure to bump the version numbers in any modules where required and update the appropriate
   README or other documentation if applicable.

4. Build the affected components and run any relevant tests before submitting the pull request.
   Bug fixes should, where practical, include a regression test or provide details of how the
   problem was reproduced and how the fix was verified.

5. Test changes on different AROS targets where practical. In particular, changes should be tested
   on different combinations of 32-bit and 64-bit, and big-endian (BE) and little-endian (LE)
   targets where the affected code supports them.

   At a minimum, contributors should consider the following target combinations:

   * 32-bit little-endian (LE)
   * 64-bit little-endian (LE)
   * 32-bit big-endian (BE)
   * 64-bit big-endian (BE)

   It is understood that contributors may not have access to every target. If a change cannot be
   tested on all relevant targets, please state which targets were tested and which could not be
   tested in the pull request.

   Take particular care with code that may be affected by architecture differences, including
   pointer sizes, integer sizes, structure layout and alignment, ABI details, and byte order.
   Do not assume that all AROS targets have the same pointer size, alignment requirements,
   structure layout, or endianness.

6. Check that the change does not introduce unnecessary compiler-specific behaviour or assumptions
   about the underlying architecture. Architecture-specific code should be clearly identified
   and isolated where practical.

7. Update documentation, examples, or other supporting files where a change affects user-visible
   behaviour, interfaces, configuration, or developer usage.

8. Ensure that the pull request description clearly explains what the change does, why it is
   required, and how it was tested. Where relevant, include the AROS targets on which the change
   was tested and identify any known limitations or untested targets.
