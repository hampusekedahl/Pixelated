import time
import sys
import os
import sqlite3
import tempfile
from PIL import Image

from optparse import OptionParser

SUPPORTED_EXTENSIONS = ('.png', '.jpg', '.jpeg')

def Log(message):
    """
    Log the given message to logfile and console
    """
    
    t = time.asctime()
    print(t + " : " + message)
    
    LogToFile(message)
    
def LogError(message):
    """
    Log the given message to error file
    """
    
    if type(message) is bytes:
        message = str(message,'UTF-8').replace("\r\n","\n")
    try:
        if message:
            errFile.write(time.asctime() + " : " + message + "\n")
    except:
        errFile.write(time.asctime() + " : LogError got a data type (" + str(type(message)) + ") that it was unable to log to file.\n")

    errFile.flush()
    
def LogToFile(message):
    """
    Log the given message to file
    """
    
    if type(message) is bytes:
        message = str(message,'UTF-8').replace("\r\n","\n")
        
    try:
        logFile.write(time.asctime() + " : " + message + "\n")
    except:
        errFile.write(time.asctime() + " : LogToFile got a data type (" + str(type(message)) + ") that it was unable to log to file.\n")
        logFile.write(time.asctime() + " : LogToFile got a data type (" + str(type(message)) + ") that it was unable to log to file.\n")
        
    logFile.flush()
    
def GetCommands(fileName):
    """
    Reads the given file for commands and return a list of commands.
    """
    commands = []
    
    try:
        lines = open(fileName,'r').readlines()
        
        in_comment_block = False

        for line in lines:
            
            line = line.strip()
            
            if not in_comment_block and len(line) > 1 and line[0] == '/' and line[1] == '*':
                in_comment_block = True
            elif in_comment_block and len(line) > 1 and line[0] == '*' and line[1] == '/':
                in_comment_block = False
            elif not in_comment_block and len(line) > 0 and line[0] != "#":
                commands.append(line.split())

    except:
        pass

    return commands
    
def ExecuteCommand(command):
    """
    Executes the given command. Valid commands are:
    - open_db
    - close_db
    - import_images_from_directory
   """

    Log("Executing: " + str(command))

    if command[0] == "open_db" and len(command) > 1:
        return OpenDatabase(command[1])
    elif command[0] == "close_db":
        return CloseDatabase()
    elif command[0] == "import_images_from_directory" and len(command) > 4:
        ImportImagesFromDir(command[1], command[2], command[3], command[4])
    else:
        Log("ExecuteCommand() - unhandle command: " + str(command))
        LogError("ExecuteCommand() - unhandle command: " + str(command))
        return False

    return True

def OpenDatabase(databaseFile):
    """
    Open/Create sqlite3 database file
    """
    global DATABASE_CONNECTION
    try:
        DATABASE_CONNECTION = sqlite3.connect(databaseFile)
        Log("OpenDatabase() - Opened OK : " + str(databaseFile))
    except Exception as e:
        Log("OpenDatabase() - Got exception: " + str(e))
        LogError("OpenDatabase() - Got exception: " + str(e))
        return False
    
    return True

def CloseDatabase():
    """
    Close sqlite3 database file
    !!!will fail if open_db has not been called!!!
    """
    try:
        DATABASE_CONNECTION.close()
        Log("CloseDatabase() - Closed OK")
    except Exception as e:
        Log("CloseDatabase() - Got exception: " + str(e))
        LogError("CloseDatabase() - Got exception: " + str(e))
        return False
    
    return True
    
def resize_image(input_path, final_size=(1200, 800), fill_color=(0, 0, 0)):
    with Image.open(input_path) as img:
        aspect = img.width / img.height
        if aspect > 1:
            new_w, new_h = final_size[0], int(final_size[0] / aspect)
        else:
            new_h, new_w = final_size[1], int(final_size[1] * aspect)

        resized = img.resize((new_w, new_h), Image.LANCZOS)
        result = Image.new("RGB", final_size, fill_color)
        result.paste(resized, ((final_size[0] - new_w) // 2, (final_size[1] - new_h) // 2))
        return result

def ImportImagesFromDir(imageDir, categoryName, width, height):
    """
    Resize and import images into the global DATABASE_CONNECTION
    """
    try:
        width = int(width)
        height = int(height)
        cursor = DATABASE_CONNECTION.cursor()

        # Ensure table exists
        cursor.execute("""
            CREATE TABLE IF NOT EXISTS images (
                id INTEGER PRIMARY KEY,
                name TEXT NOT NULL,
                category TEXT,
                data BLOB NOT NULL
            )
        """)

        for root, _, files in os.walk(imageDir):
            for file in files:
                if not file.lower().endswith(SUPPORTED_EXTENSIONS):
                    continue

                try:
                    full_path = os.path.join(root, file)
                    image = resize_image(full_path, final_size=(width, height))
                    name = os.path.splitext(file)[0]

                    with tempfile.NamedTemporaryFile(suffix=".jpg", delete=False) as tmp:
                        image.save(tmp.name)
                        with open(tmp.name, 'rb') as f:
                            blob = f.read()

                    cursor.execute(
                        "INSERT INTO images (name, category, data) VALUES (?, ?, ?)",
                        (name, categoryName, blob)
                    )
                    Log(f"[+] Imported image: {file}, Category : {categoryName}")
                    os.remove(tmp.name)

                except Exception as e:
                    Log(f"[!] Failed to import {file}: {e}")
                    LogError(f"[!] Failed to import {file}: {e}")

        DATABASE_CONNECTION.commit()
        Log("Image import complete.")

    except Exception as e:
        Log("ImportImagesFromDir() - Got exception: " + str(e))
        LogError("ImportImagesFromDir() - Got exception: " + str(e))

def main():
    """
    Main program.
    """
    
    parser = OptionParser("usage: %prog [options]")
    parser.add_option("-c", "--cmd", dest="commands",
                      default="", help="command file")
        
    (options, args) = parser.parse_args()
    
    global logFile
    global errFile

    logFile = open("full_log.txt","w")
    errFile = open("error_log.txt","w")
    
    if not os.path.isfile(options.commands):
        parser.print_help()
        sys.exit(1)
        
    Log("Using command file: " + str(options.commands))
    
    commands = GetCommands(options.commands)
    
    for command in commands:
    
        if command[0] == "exit":
            return
            
        if not ExecuteCommand(command):
            Log("ExecuteCommand() - cmd failed: " + str(command))
            LogError("ExecuteCommand() - cmd failed: " + str(command))
            
            sys.exit(-1)
            return
    
if __name__ == '__main__':
    sys.exit(main())