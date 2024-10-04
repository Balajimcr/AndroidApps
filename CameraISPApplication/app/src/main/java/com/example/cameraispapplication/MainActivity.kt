package com.example.cameraispapplication

import android.app.Activity
import android.content.Intent
import android.graphics.BitmapFactory
import android.net.Uri
import android.os.Bundle
import android.widget.Toast
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import android.Manifest
import com.example.cameraispapplication.databinding.ActivityMainBinding

class MainActivity : AppCompatActivity() {
    private val REQUEST_CODE = 100
    private lateinit var binding: ActivityMainBinding
    private var inputFilePath: String? = null
    private var outputFilePath: String? = null

    // Native method to process the image (apply Gaussian blur in the C++ code)
    external fun processImage(inputPath: String, outputPath: String)

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        // Request permission to access external storage
        val permissions = arrayOf(Manifest.permission.READ_EXTERNAL_STORAGE)
        ActivityCompat.requestPermissions(this, permissions, REQUEST_CODE)

        // Set up button click listener for file browsing
        binding.BrowseBtn.setOnClickListener {
            openFilePicker()
        }

        // Set up button click listener for processing image
        binding.processButton.setOnClickListener {
            if (inputFilePath == null) {
                Toast.makeText(this, "Please select an input BMP file first.", Toast.LENGTH_LONG).show()
            } else {
                outputFilePath = inputFilePath?.replace(".bmp", "_output.bmp")

                // Process the image using the native C++ method
                processImage(inputFilePath!!, outputFilePath!!)

                // Display the processed output image in the ImageView
                displayImage(outputFilePath!!, binding.inputImageView)  // Reusing the same ImageView for output

                Toast.makeText(this, "Image processed successfully!", Toast.LENGTH_LONG).show()
                Toast.makeText(this, "Output saved to: $outputFilePath", Toast.LENGTH_LONG).show()
            }
        }
    }

    /**
     * A native method that is implemented by the 'cameraispapplication' native library,
     * which is packaged with this application.
     */

    companion object {
        // Used to load the 'cameraispapplication' library on application startup.
        init {
            System.loadLibrary("cameraispapplication")
        }
    }

    // Function to open the file picker
    private fun openFilePicker() {
        val intent = Intent(Intent.ACTION_GET_CONTENT)
        intent.type = "image/bmp" // Filter for BMP files
        intent.addCategory(Intent.CATEGORY_OPENABLE)
        filePickerLauncher.launch(intent)
    }

    // Handle the result of the file picker
    private val filePickerLauncher =
        registerForActivityResult(ActivityResultContracts.StartActivityForResult()) { result ->
            if (result.resultCode == Activity.RESULT_OK && result.data != null) {
                val uri: Uri? = result.data?.data
                uri?.let {
                    inputFilePath = getPathFromUri(it)
                    Toast.makeText(this, "File selected: $inputFilePath", Toast.LENGTH_LONG).show()

                    // Display the input image in the ImageView
                    displayImage(inputFilePath!!, binding.inputImageView)
                }
            }
        }

    // Helper function to get the real file path from URI
    private fun getPathFromUri(uri: Uri): String? {
        var path: String? = null
        contentResolver.query(uri, null, null, null, null)?.use { cursor ->
            val columnIndex = cursor.getColumnIndexOrThrow("_data")
            cursor.moveToFirst()
            path = cursor.getString(columnIndex)
        }
        return path
    }

    // Function to display image in the ImageView
    private fun displayImage(filePath: String, imageView: android.widget.ImageView) {
        val bitmap = BitmapFactory.decodeFile(filePath)
        if (bitmap != null) {
            imageView.setImageBitmap(bitmap)
        } else {
            Toast.makeText(this, "Unable to load image.", Toast.LENGTH_LONG).show()
        }
    }
}
