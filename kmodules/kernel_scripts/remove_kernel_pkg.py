import os

def main():
    current_path = os.getcwd()
    files = os.listdir(current_path)

    to_be_deleted_files = []
    for file in files:
        if file.startswith("linux-upstream"):
            to_be_deleted_files.append(file)
        elif file.startswith("linux-headers") or file.startswith("linux-libc") or file.startswith("linux-image"):
            to_be_deleted_files.append(file)

    print("The following files will be deleted:")
    for file in to_be_deleted_files:
        print("  ", file)

    yes_or_no = input("Are you sure you want to delete these files? (y/n): ")
    if yes_or_no.lower() == "y":
        for file in to_be_deleted_files:
            os.remove(file)
        print("Files deleted successfully.")
    else:
        print("Files not deleted.")

if __name__ == "__main__":
    main()