#include <git2.h>
#include <string>

typedef struct {} diff_data;

int each_file_cb(const git_diff_delta *delta, float progress, void *payload)
{
    return 0;
}

int each_binary_cb(const git_diff_delta *delta, const git_diff_binary *binary, void *payload)
{
    return 0;
}

int each_hunk_cb(const git_diff_delta *delta, const git_diff_hunk * hunk, void *payload)
{
    std::string filePath(delta->new_file.path);

    if (delta->new_file.path && (filePath.substr(filePath.size() - 4) == ".cpp" || filePath.substr(filePath.size() - 2) == ".c")) {
        if (hunk->new_start > 0) {
            for(int i = 0; i < hunk->new_lines; i++) {
                printf("%s,%d\n", delta->new_file.path, hunk->new_start + i);
            }
        }
    }

    return 0;
}
int each_line_cb(const git_diff_delta *delta, const git_diff_hunk *hunk, const git_diff_line *line, void *payload)
{
    // printf("LINE: %s %d, %d\n", delta->new_file.path, line->new_lineno, line->num_lines);

    return 0;
}

int main(int argc, char const **argv)
{
    git_libgit2_init();

    if (argc < 2) {
        return 1;
    }

    std::string repo_path = argv[1];

    git_repository *repo = NULL;

    if (git_repository_open(&repo, repo_path.c_str())) {
        return 1;
    }

    git_oid oid;
    if (git_reference_name_to_id(&oid, repo, "HEAD")) {
        return 1;
    }

    //printf("HEAD oid: %s\n", git_oid_tostr_s(&oid));

    git_commit * commit;
    if (git_commit_lookup(&commit, repo, &oid)) {
        return 1;
    }

    git_commit *parent;
    if (git_commit_parent(&parent, commit, 0)) {
        return 1;
    }

    git_tree *commit_tree, *parent_tree;
    if (git_commit_tree(&commit_tree, commit)) {
        return 1;
    }

    if (git_commit_tree(&parent_tree, parent)) {
        return 1;
    }

    git_diff *diff;
    if (git_diff_tree_to_tree(&diff, repo, parent_tree, commit_tree, NULL)) {
        return 1;
    }

    diff_data d;

    if (git_diff_foreach(diff, each_file_cb, each_binary_cb, each_hunk_cb, each_line_cb, &d)) {

    }

    //printf("HEAD summary: %s\n", git_commit_summary(commit)); 
    //printf("HEAD body: %s\n", git_commit_body(commit)); 
   
    return 0;
}