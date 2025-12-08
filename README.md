# Student Manager

A modern Qt6/C++ application for managing student data with Google Firestore integration.

## Features

- **Modern UI**: Dark theme with professional styling
- **Student Management**: Add, edit, delete, and view student records
- **Firestore Integration**: Cloud-based storage using Google Firestore
- **Search & Filter**: Real-time search through student data
- **Data Validation**: Form validation for student information
- **Responsive Design**: Split-pane layout with detailed student view
- **Excel Import/Export**: Import and export student data via Excel files
- **Statistics**: View comprehensive statistics about student data
- **Auto-Update System**: Automatic update checking via GitHub Releases
- **Firebase Authentication**: Secure user authentication
- **Photo Upload**: Upload and manage student photos via Firebase Storage

## Student Data Structure

The application manages the following student information:
- **Personal**: Name, Email, Description, Photo URL
- **Academic**: Field of study, School, Year, Graduation status
- **Contact**: Phone number

## Prerequisites

- Qt6 (Core, Widgets, Network modules)
- CMake 3.16 or higher
- C++17 compatible compiler
- Google Firestore project with REST API access

## Building

1. Clone the repository
2. Create a build directory:
   ```bash
   mkdir build && cd build
   ```
3. Configure with CMake:
   ```bash
   cmake ..
   ```
4. Build the project:
   ```bash
   cmake --build .
   ```

## Firestore Setup

1. Create a Google Cloud Project
2. Enable the Firestore API
3. Create a Firestore database
4. Get your project ID and API key (optional for public access)
5. Configure the application:
   - Go to File → Settings
   - Enter your Firestore Project ID
   - Optionally enter API key for authenticated access

## Database Structure

Students are stored in Firestore under the collection `students` with the following structure:

```json
{
  "id": "unique-student-id",
  "name": "Student Name",
  "email": "student@example.com",
  "description": "EMK Üyesi",
  "field": "Elektrik-Elektronik Mühendisliği",
  "school": "TED Üniversitesi",
  "number": 5541542293,
  "year": 2023,
  "graduation": false,
  "photoURL": "https://storage.googleapis.com/..."
}
```

## Usage

1. **Launch the application**
2. **Configure Firestore**: File → Settings → Enter Project ID
3. **Load students**: Click Refresh or restart the application
4. **Add students**: Click "Add Student" button
5. **Edit students**: Double-click a row or select and click "Edit Student"
6. **Delete students**: Select a row and click "Delete Student"
7. **Search**: Use the search box to filter students by name, email, field, etc.

## Architecture

- **Student**: Data model class for student information
- **FirestoreService**: Handles all Firestore REST API communication
- **FirebaseAuthService**: Manages user authentication
- **FirebaseStorageService**: Handles file uploads to Firebase Storage
- **MainWindow**: Main application window with student list and details
- **StudentDialog**: Modal dialog for adding/editing students
- **StatisticsDialog**: Displays comprehensive statistics and charts
- **UpdateChecker**: Checks for application updates from GitHub Releases
- **UpdateDialog**: Displays update information to users

## Error Handling

The application includes comprehensive error handling for:
- Network connectivity issues
- Firestore API errors
- Data validation errors
- JSON parsing errors

## Deployment

### Windows

Use the provided deployment script to create a distributable package:

```bash
deploy_windows.bat
```

This will create a `deploy_windows` folder with all necessary Qt libraries and dependencies.

For more details, see [DEPLOYMENT_GUIDE.md](DEPLOYMENT_GUIDE.md).

## Auto-Update System

The app includes automatic updates from GitHub Releases.

**Setup:**
1. Edit `src/mainwindow.cpp` line ~1690: Set your GitHub repo path `"username/repo"`
2. Update version: `update_version.bat 1.0.1`
3. Release: `git tag v1.0.1 && git push origin v1.0.1`
4. GitHub Actions builds & publishes automatically

**Users:** Go to Help → Check for Updates → Click "Install Now" - the app downloads, extracts, and installs automatically.

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests if applicable
5. Submit a pull request

## Support

For support or questions:
- Create an issue on GitHub
- Contact: FURKAN KARAKETIR +90 551 145 09 68

## License

This project is licensed under the MIT License.
