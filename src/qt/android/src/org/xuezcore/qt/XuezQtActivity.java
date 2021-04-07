package org.xuezcore.qt;
import android.Manifest;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.system.ErrnoException;
import android.system.Os;
import android.os.Environment;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.widget.Toast;
import org.qtproject.qt5.android.bindings.QtActivity;
import java.io.File;

public class XuezQtActivity extends QtActivity
{
    @Override
    public void onCreate(Bundle savedInstanceState)
    {

        if (ContextCompat.checkSelfPermission(this, Manifest.permission.READ_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.READ_EXTERNAL_STORAGE}, 1);
        }

        if (ContextCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE}, 1);
        }

        String dPath = Environment.getExternalStorageDirectory().getAbsolutePath() + "/" + getPackageName();
        final File xuezDir = new File(Environment.getExternalStorageDirectory().getAbsolutePath() + "/" + getPackageName());
        String msg = "Created directory: " + dPath;
        if ( !xuezDir.isDirectory() ) {
            try {
              xuezDir.mkdir();
            } catch(Exception e){
              msg = "Failed to create: " + dPath + e.toString();
            }
            Toast.makeText(this, msg, Toast.LENGTH_LONG).show();
        }
        try {
            Os.setenv("QT_QPA_PLATFORM", "android", true);
        } catch (ErrnoException e) {
            e.printStackTrace();
        }

        super.onCreate(savedInstanceState);
    }
}
