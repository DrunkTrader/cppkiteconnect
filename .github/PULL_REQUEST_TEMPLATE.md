# Pull Request Template

## Description

Please include a summary of the change and which issue is fixed. Please also include relevant motivation and context. List any dependencies that are required for this change.

Fixes # (issue)

## Type of change

Please delete options that are not relevant.

- [ ] Bug fix (non-breaking change which fixes an issue)
- [ ] New feature (non-breaking change which adds functionality)
- [ ] Breaking change (fix or feature that would cause existing functionality to not work as expected)
- [ ] Documentation update
- [ ] Performance improvement
- [ ] Code refactoring (no functional changes)
- [ ] Test update (adding/updating tests)
- [ ] Build/CI configuration change

## How Has This Been Tested?

Please describe the tests that you ran to verify your changes. Provide instructions so we can reproduce. Please also list any relevant details for your test configuration.

- [ ] Unit tests pass (`make test`)
- [ ] Integration tests pass (if applicable)
- [ ] Manual testing performed with description below
- [ ] Sanitizer tests pass (ASan, UBSan, TSan)
- [ ] Benchmark tests run (if performance changes)

**Test Configuration**:
- OS: 
- Compiler: 
- CMake version: 
- Boost version (if applicable): 

## Checklist

Before submitting this PR, please ensure the following:

- [ ] My code follows the style guidelines of this project
- [ ] I have performed a self-review of my own code
- [ ] I have commented my code, particularly in hard-to-understand areas
- [ ] I have made corresponding changes to the documentation
- [ ] My changes generate no new warnings
- [ ] I have added tests that prove my fix is effective or that my feature works
- [ ] New and existing unit tests pass locally with my changes
- [ ] Any dependent changes have been merged and published in downstream modules
- [ ] I have updated the CHANGELOG.md (if applicable)
- [ ] I have checked that my changes don't introduce memory leaks (sanitizer clean)
- [ ] For new features: I have added comprehensive examples

## Code Quality

- [ ] Code is properly formatted (clang-format)
- [ ] No unnecessary includes or dependencies added
- [ ] Error handling is comprehensive
- [ ] Thread safety considerations addressed (if applicable)

## Additional Notes

Add any additional information that reviewers should know about this PR.

## Screenshots/Recordings (if applicable)

If your changes affect the user interface or output, please include screenshots or recordings demonstrating the changes.

---

**For Maintainers:**
- [ ] Code review completed
- [ ] All CI checks passed
- [ ] Documentation reviewed
- [ ] Ready to merge
