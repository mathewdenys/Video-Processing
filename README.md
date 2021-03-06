# Video-Previewer

A GUI for previewing video files on macOS, intended for searching through long videos.

## Requirements

- Processing of video files on the backend is acheived with  [OpenCV](https://opencv.org/). Dynamic libraries can be installed using `brew install opencv`
  - If you have `brew` set up to install to a location other than `/usr/local/Cellar`, you will need to adust the build settings in the XCode project. In particular, the *Header Search Paths* and *Library Search Paths*, as well as *Other Linker Flags*
- The project currently builds on Xcode 12.4; there is an issue with the Swift code which leads to cimpilation errors on some previous versions of Xcode

## Example files

See [here](https://github.com/mathewdenys/Video-Previewer-Files) for some example videos (for previewing) and an example configuration file

## Configuration Files

### Format

- Each line of the configuration file is expected to be of the form 

  ```
  option_id = option_value
  ```

- All whitespace is ignored (except new lines), so all of the following are valid, and interpreted as above

  ```
  option_id =option_value
  option_id= option_value
  option_id=option_value
  opti on_ id= o p t i o n _ v a l u e
  ```

  - Corollary: the last possibility in particular is recommended against

- Comments are indicated by a hash, `#`

  - Any line beginning with hash will be ignored
  - Any text after a hash on a given line will be ignored

### Recognised Options & Values

| ID                 | Allowed values                            | Default value |
| ------------------ | ----------------------------------------- | ------------- |
| frames_to_show     | Any number between 0.0 and 1.0, or "auto" | 0.5           |
| maximum_frames     | Positive integers, or "auto"              | "auto"        |
| minimum_sampling   | Positive integers                         | 25            |
| maximum_percentage | An integer between 0 and 100              | 20            |
| overlay_timestamp  | "true" or" false"                         | "true"        |
| overlay_number     | "true" of "false"                         | "false"       |
| frame_size         | A number between 0.0 and 1.0              | 0.25          |

#### Unrecognised options & invalid values

For compatibility with future versions, any option parsed from a configuration file with an unrecognised ID (i.e. not one of those listed above) will be stored internally, and will be included when configuration options are exported to a file.

Similarly, any *recognised* option with an invalid value (i.e. one not listed under *Allowed values* above) will be saved internally, but ignored by the program. If the same option is also parsed from a different configuration file with a valid value, that value will be used.

### File locations

- Local configuration files, named `.videopreviewconfig`, can be placed in any directory
  - Any configuration file between the working directory and the user home (or root, if the working directory is not a child of the home directory) will be searched for configuration files
- User options are set in `$HOME/.config/videopreview`
- System wide / global options are set in `/etc/videopreviewconfig`

#### Priority

- Options defined in local config files take precedent over user options, which take precedent over global options
  - Local files: those lower in the directory hieracy are prioritised
    - e.g. `$HOME/project/videos/.videopreviewconfig` is prioritised over `$HOME/project/.videopreviewconfig`
  - Lower priority files are still parsed. Any options that aren't defined in higher priority files are implemented
- Within a given configuration file, options closer the top are prioritised (i.e if there is a duplicate option, the second version will be ignored)